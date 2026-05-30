#include "packimporttask.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QUrl>
#include <mcpelauncher/zip_extractor.h>

PackImportTask::PackImportTask(QObject *parent) : QThread(parent) {
    connect(this, &QThread::started, this, &PackImportTask::emitActiveChanged);
    connect(this, &QThread::finished, this, &PackImportTask::emitActiveChanged);
}

QStringList PackImportTask::sources() {
    QMutexLocker locker(&mutex);
    return m_sources;
}

void PackImportTask::setSources(QStringList const& value) {
    QMutexLocker locker(&mutex);
    m_sources = value;
}

QString PackImportTask::gameDataDir() {
    QMutexLocker locker(&mutex);
    return m_gameDataDir;
}

void PackImportTask::setGameDataDir(QString const& value) {
    QMutexLocker locker(&mutex);
    m_gameDataDir = value;
}

bool PackImportTask::setSourceUrls(QList<QUrl> const& urls) {
    QStringList list;
    for (auto&& url : urls) {
        if (!url.isLocalFile()) {
            return false;
        }
        list.append(url.toLocalFile());
    }
    setSources(list);
    return true;
}

static QString sanitizeName(QString name, QString fallback) {
    name = name.trimmed();
    if (name.isEmpty()) {
        name = fallback;
    }
    QString out;
    for (auto ch : name) {
        if (ch.isLetterOrNumber() || ch == '-' || ch == '_' || ch == '.' || ch == ' ') {
            out.append(ch);
        } else {
            out.append('_');
        }
    }
    out = out.trimmed();
    return out.isEmpty() ? fallback : out;
}

static QString uniqueDirectory(QString parentDir, QString desiredName) {
    QDir parent(parentDir);
    QString cleanName = sanitizeName(desiredName, "imported");
    QString candidate = parent.filePath(cleanName);
    int suffix = 1;
    while (QFileInfo::exists(candidate)) {
        candidate = parent.filePath(QString("%1 (%2)").arg(cleanName).arg(suffix++));
    }
    return candidate;
}

static void copyDirectory(QString from, QString to) {
    QDir source(from);
    if (!source.exists()) {
        throw std::runtime_error(QObject::tr("Source folder does not exist: %1").arg(from).toStdString());
    }
    if (!QDir().mkpath(to)) {
        throw std::runtime_error(QObject::tr("Could not create folder: %1").arg(to).toStdString());
    }
    for (auto const& entry : source.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
        QString target = QDir(to).filePath(entry.fileName());
        if (entry.isDir()) {
            copyDirectory(entry.absoluteFilePath(), target);
        } else if (entry.isFile()) {
            QFile::remove(target);
            if (!QFile::copy(entry.absoluteFilePath(), target)) {
                throw std::runtime_error(QObject::tr("Could not copy %1").arg(entry.absoluteFilePath()).toStdString());
            }
        }
    }
}

static bool extractArchive(QString source, QString targetDir, std::function<void(qreal)> progress) {
    ZipExtractor extractor(source.toStdString());
    QString base = QFileInfo(targetDir).absoluteFilePath();
    QDir().mkpath(base);
    extractor.extractTo(
        [&base](const char* filename, std::string& outName) -> bool {
            QString entry = QString::fromUtf8(filename);
            if (entry.endsWith('/')) {
                return false;
            }
            QString target = QFileInfo(QDir(base).filePath(entry)).absoluteFilePath();
            if (!target.startsWith(base + "/")) {
                throw std::runtime_error(QObject::tr("Archive contains an unsafe path: %1").arg(entry).toStdString());
            }
            outName = target.toStdString();
            return true;
        },
        [&progress](size_t current, size_t max, ZipExtractor::FileHandle const&, size_t, size_t) {
            progress(max == 0 ? 1 : (qreal) current / max);
        }
    );
    return true;
}

static QJsonObject readManifest(QString path) {
    QFile file(QDir(path).filePath("manifest.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return {};
    }
    return doc.object();
}

static QString manifestName(QJsonObject const& manifest, QString fallback) {
    auto header = manifest.value("header").toObject();
    return sanitizeName(header.value("name").toString(), fallback);
}

static QString packType(QJsonObject const& manifest) {
    auto modules = manifest.value("modules").toArray();
    for (auto const& moduleValue : modules) {
        auto type = moduleValue.toObject().value("type").toString();
        if (type == "resources") {
            return "resource";
        }
        if (type == "data") {
            return "behavior";
        }
    }
    return {};
}

static void importExtractedPath(QString path, QString gameDataDir, int& worlds, int& resources, int& behaviors);

static void importPackFolder(QString path, QString gameDataDir, QJsonObject const& manifest, int& resources, int& behaviors) {
    QString type = packType(manifest);
    QString parent;
    if (type == "resource") {
        parent = QDir(gameDataDir).filePath("games/com.mojang/resource_packs");
    } else if (type == "behavior") {
        parent = QDir(gameDataDir).filePath("games/com.mojang/behavior_packs");
    } else {
        throw std::runtime_error(QObject::tr("Pack manifest does not declare a resource or behavior module").toStdString());
    }
    QDir().mkpath(parent);
    QString target = uniqueDirectory(parent, manifestName(manifest, QFileInfo(path).completeBaseName()));
    copyDirectory(path, target);
    if (type == "resource") {
        resources++;
    } else {
        behaviors++;
    }
}

static bool isArchiveFile(QString path) {
    QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == "mcpack" || suffix == "mcworld" || suffix == "mcaddon" || suffix == "mctemplate" || suffix == "zip";
}

static void importExtractedPath(QString path, QString gameDataDir, int& worlds, int& resources, int& behaviors) {
    QDir dir(path);
    if (QFileInfo(dir.filePath("level.dat")).exists()) {
        QString parent = QDir(gameDataDir).filePath("games/com.mojang/minecraftWorlds");
        QDir().mkpath(parent);
        QString target = uniqueDirectory(parent, QFileInfo(path).completeBaseName());
        copyDirectory(path, target);
        worlds++;
        return;
    }

    auto manifest = readManifest(path);
    if (!manifest.isEmpty()) {
        importPackFolder(path, gameDataDir, manifest, resources, behaviors);
        return;
    }

    bool importedChild = false;
    QString lastError;
    for (auto const& entry : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
        try {
            if (entry.isDir()) {
                int before = worlds + resources + behaviors;
                importExtractedPath(entry.absoluteFilePath(), gameDataDir, worlds, resources, behaviors);
                importedChild = importedChild || before != worlds + resources + behaviors;
            } else if (entry.isFile() && isArchiveFile(entry.absoluteFilePath())) {
                QTemporaryDir nested;
                extractArchive(entry.absoluteFilePath(), nested.path(), [](qreal) {});
                int before = worlds + resources + behaviors;
                importExtractedPath(nested.path(), gameDataDir, worlds, resources, behaviors);
                importedChild = importedChild || before != worlds + resources + behaviors;
            }
        } catch (std::exception& e) {
            lastError = QString::fromStdString(e.what());
        }
    }

    if (!importedChild) {
        if (!lastError.isEmpty()) {
            throw std::runtime_error(lastError.toStdString());
        }
        throw std::runtime_error(QObject::tr("No world, resource pack, or behavior pack was found in %1").arg(QFileInfo(path).fileName()).toStdString());
    }
}

void PackImportTask::run() {
    m_importedWorlds = 0;
    m_importedResourcePacks = 0;
    m_importedBehaviorPacks = 0;

    try {
        auto sourceList = sources();
        auto dataDir = gameDataDir();
        if (sourceList.isEmpty()) {
            throw std::runtime_error(QObject::tr("No files selected").toStdString());
        }
        if (dataDir.isEmpty()) {
            throw std::runtime_error(QObject::tr("Game data directory is not available").toStdString());
        }

        for (int i = 0; i < sourceList.size(); i++) {
            QString source = sourceList.at(i);
            QTemporaryDir tempDir;
            extractArchive(source, tempDir.path(), [this, i, sourceList](qreal value) {
                emit progress(((qreal) i + value) / sourceList.size());
            });
            importExtractedPath(tempDir.path(), dataDir, m_importedWorlds, m_importedResourcePacks, m_importedBehaviorPacks);
        }
    } catch (std::exception& e) {
        emit error(e.what());
        return;
    }

    QStringList parts;
    if (m_importedWorlds) {
        parts.append(QObject::tr("%n world(s)", nullptr, m_importedWorlds));
    }
    if (m_importedResourcePacks) {
        parts.append(QObject::tr("%n resource pack(s)", nullptr, m_importedResourcePacks));
    }
    if (m_importedBehaviorPacks) {
        parts.append(QObject::tr("%n behavior pack(s)", nullptr, m_importedBehaviorPacks));
    }
    emit importFinished(QObject::tr("Imported %1").arg(parts.join(", ")));
}
