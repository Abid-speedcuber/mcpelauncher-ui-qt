#include "versionmanager.h"

#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include "highdistanceassets.h"

#ifndef LAUNCHER_VERSIONDB_URL
#define LAUNCHER_VERSIONDB_URL "https://raw.githubusercontent.com/minecraft-linux/mcpelauncher-versiondb/master"
#endif

VersionManager::VersionManager() : m_versionList(m_versions) {
    baseDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath("mcpelauncher/versions");
    instancesDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath("mcpelauncher/instances");
    QDir().mkpath(baseDir);
    QDir().mkpath(instancesDir);
    loadVersions();
}

#include "supportedandroidabis.h"

void VersionManager::loadVersions() {
    QSettings settings(QDir(baseDir).filePath("versions.ini"), QSettings::IniFormat);
    bool metadataChanged = false;
    for (QString const& group : settings.childGroups()) {
        settings.beginGroup(group);
        int size = settings.beginReadArray("codes");
        if (size >= 1) {
            auto ver = new VersionInfo(this);
            int i = 0;
            while (i < size) {
                settings.setArrayIndex(i++);
                auto versionCode = settings.value("code").toInt();
                ver->codes.insert(settings.value("arch").toString(), versionCode);
            }
            settings.endArray();
            ver->directory = group;
            ver->dataDirectory = settings.value("dataDirectory").toString();
            ver->versionName = settings.value("versionName").toString();
            ver->instanceName = settings.value("instanceName", ver->versionName).toString();
            ver->customNamed = settings.value("customNamed", false).toBool();
            m_versions[group] = ver;
            metadataChanged |= normalizeDataDirectory(ver);
            applyHighDistanceAssetPatches(getDirectoryFor(ver));
        } else {
            settings.endArray();
            // Migrate previous format
            bool ok = false;
            int versionCode = settings.value("versionCode").toInt(&ok);
            if (ok) {
                auto& ver = m_versions[group];
                if (ver == nullptr)
                    ver = new VersionInfo(this);
                ver->directory = group;
                ver->dataDirectory = settings.value("dataDirectory").toString();
                ver->versionName = settings.value("versionName").toString();
                ver->instanceName = settings.value("instanceName", ver->versionName).toString();
                ver->customNamed = settings.value("customNamed", false).toBool();
                metadataChanged |= normalizeDataDirectory(ver);
                for (auto &&abi : SupportedAndroidAbis::getAbis()) {
                    if (QFile(getDirectoryFor(ver->directory) + "/lib/" + QString::fromStdString(abi.first) + "/libminecraftpe.so").exists()) {
                        ver->codes[QString::fromStdString(abi.first)] = versionCode;
                    }
                }
                applyHighDistanceAssetPatches(getDirectoryFor(ver));
            }
        }
        
        settings.endGroup();
    }
    if (metadataChanged)
        saveVersions();
}

void VersionManager::saveVersions() {
    QString settingsPath = QDir(baseDir).filePath("versions.ini");
    QString backupPath = settingsPath + ".bak";
    if (QFileInfo::exists(settingsPath)) {
        QFile::remove(backupPath);
        QFile::copy(settingsPath, backupPath);
    }
    QSettings settings(settingsPath, QSettings::IniFormat);
    settings.clear();
    for (auto const& ver : m_versions) {
        if (!ver)
            continue;
        settings.beginGroup(ver->directory);
        int i = 0;
        settings.setValue("versionName", ver->versionName);
        settings.setValue("versionCode", ver->versionCode());
        settings.setValue("dataDirectory", ver->dataDirectory);
        settings.setValue("instanceName", ver->instanceName);
        settings.setValue("customNamed", ver->customNamed);
        auto size = ver->codes.size();
        settings.beginWriteArray("codes", size);
        QHash<QString, int>::const_iterator it = ver->codes.constBegin();
        while (it != ver->codes.constEnd()) {
            settings.setArrayIndex(i++);
            settings.setValue("code", it.value());
            settings.setValue("arch", it.key());
            ++it;
        }
        settings.endArray();
        settings.endGroup();
    }
    settings.sync();
}

bool VersionManager::normalizeDataDirectory(VersionInfo* version) {
    if (!version || version->dataDirectory.isEmpty())
        return false;

    QString stored = QDir::cleanPath(version->dataDirectory);
    QString currentInstances = QDir::cleanPath(instancesDir);
    if (stored == currentInstances || stored.startsWith(currentInstances + "/")) {
        if (stored == version->dataDirectory)
            return false;
        version->dataDirectory = stored;
        return true;
    }

    QString marker = "/mcpelauncher/instances/";
    int markerIndex = stored.lastIndexOf(marker);
    if (markerIndex < 0 || QFileInfo::exists(stored))
        return false;

    QString relativePath = QDir::cleanPath(stored.mid(markerIndex + marker.size()));
    if (relativePath.isEmpty() || relativePath == "." || relativePath == ".." ||
        relativePath.startsWith("../") || QDir::isAbsolutePath(relativePath))
        return false;

    version->dataDirectory = QDir(currentInstances).filePath(relativePath);
    qWarning() << "Rebased stale instance data path" << stored << "to" << version->dataDirectory;
    return true;
}

QString VersionManager::getTempTemplate() {
    return QDir(getBaseDir()).filePath("temp-XXXXXX");
}

QString VersionManager::getDirectoryFor(QString const& directory) {
    return QDir(getBaseDir()).filePath(directory);
}

QString VersionManager::getDirectoryFor(std::string const& directory) {
    return getDirectoryFor(QString::fromStdString(directory));
}

QString VersionManager::getDirectoryFor(VersionInfo *version) {
    if (version == nullptr)
        return QString();
    return getDirectoryFor(version->directory);
}

static QString sanitizeInstanceName(QString name, QString fallback) {
    name = name.trimmed();
    if (name.isEmpty())
        name = fallback;
    QString out;
    for (auto ch : name) {
        if (ch.isLetterOrNumber() || ch == '-' || ch == '_' || ch == '.')
            out.append(ch);
        else if (ch.isSpace())
            out.append('-');
        else
            out.append('_');
    }
    out = out.trimmed();
    return out.isEmpty() ? fallback : out;
}

QString VersionManager::createUniqueDirectoryName(QString desiredName) const {
    QDir parent(baseDir);
    QString baseName = sanitizeInstanceName(desiredName, "minecraft");
    QString candidate = baseName;
    int suffix = 2;
    while (m_versions.contains(candidate) || QFileInfo::exists(parent.filePath(candidate)) ||
           QFileInfo::exists(QDir(instancesDir).filePath(candidate))) {
        candidate = QString("%1-%2").arg(baseName).arg(suffix++);
    }
    return candidate;
}

void VersionManager::addVersion(QString directory, QString versionName, int versionCode, QString dataDirectory,
                                QString instanceName, bool customNamed) {
    auto& ver = m_versions[directory];
    bool isNew = ver == nullptr;
    if (isNew)
        ver = new VersionInfo(this);
    ver->directory = directory;
    if (dataDirectory.isEmpty() && ver->dataDirectory.isEmpty())
        dataDirectory = QDir(instancesDir).filePath(directory + "/data");
    if (!dataDirectory.isEmpty())
        ver->dataDirectory = dataDirectory;
    ver->versionName = versionName;
    if (isNew) {
        ver->customNamed = customNamed && !instanceName.trimmed().isEmpty();
        ver->instanceName = ver->customNamed ? instanceName.trimmed() : versionName;
    } else if (!ver->customNamed) {
        ver->instanceName = versionName;
    }
    ver->codes.clear();
    for (auto &&abi : SupportedAndroidAbis::getAbis()) {
        auto && it = ver->codes.constFind(QString::fromStdString(abi.first));
        if (it == ver->codes.constEnd() && QFile(getDirectoryFor(ver->directory) + "/lib/" + QString::fromStdString(abi.first) + "/libminecraftpe.so").exists()) {
            ver->codes[QString::fromStdString(abi.first)] = versionCode;
        }
    }
    saveVersions();
    emit ver->metadataChanged();
    emit versionListChanged();
}

QString VersionManager::getDataDirectoryFor(VersionInfo* version) {
    if (version == nullptr)
        return QString();
    if (normalizeDataDirectory(version)) {
        saveVersions();
        emit version->metadataChanged();
    }
    if (!version->dataDirectory.isEmpty())
        return version->dataDirectory;
    // Existing installs predate per-instance data. Keep them on the old shared
    // data root until the user duplicates or reimports.
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath("mcpelauncher");
}

QString VersionManager::getWorldsDirectoryFor(VersionInfo* version) {
    return QDir(getDataDirectoryFor(version)).filePath("games/com.mojang/minecraftWorlds");
}

QString VersionManager::getResourcePacksDirectoryFor(VersionInfo* version) {
    return QDir(getDataDirectoryFor(version)).filePath("games/com.mojang/resource_packs");
}

QString VersionManager::getBehaviorPacksDirectoryFor(VersionInfo* version) {
    return QDir(getDataDirectoryFor(version)).filePath("games/com.mojang/behavior_packs");
}

static bool copyDirectoryRecursive(QString const& from, QString const& to) {
    QDir source(from);
    if (!source.exists())
        return true;
    if (!QDir().mkpath(to))
        return false;
    for (auto const& entry : source.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
        QString target = QDir(to).filePath(entry.fileName());
        if (entry.isDir()) {
            if (!copyDirectoryRecursive(entry.absoluteFilePath(), target))
                return false;
        } else if (entry.isFile()) {
            if (QFileInfo::exists(target) && !QFile::remove(target))
                return false;
            if (!QFile::copy(entry.absoluteFilePath(), target))
                return false;
        }
    }
    return true;
}

void VersionManager::deleteVersion(VersionInfo* version, bool removeData) {
    if (!version) return;
    QString directory = version->directory;
    QString dataDirectory = getDataDirectoryFor(version);
    QDir(getDirectoryFor(version)).removeRecursively();
    if (removeData)
        QDir(dataDirectory).removeRecursively();
    auto val = m_versions.find(directory);
    if (val != m_versions.end() && val.value() == version) {
        m_versions.erase(val);
        version->deleteLater();
    }
    saveVersions();
    emit versionListChanged();
}

void VersionManager::removeVersion(VersionInfo* version, QStringList abis) {
    if (!version) return;
    for (auto&& abi : abis) {
        version->codes.remove(abi);
    }
    if (version->codes.isEmpty())
        QDir(getDirectoryFor(version)).removeRecursively();
    saveVersions();
    emit versionListChanged();
}

VersionInfo* VersionManager::duplicateVersion(VersionInfo* version) {
    if (!version)
        return nullptr;
    QString newDirectory = createUniqueDirectoryName(version->directory + "-copy");
    QString newVersionDir = getDirectoryFor(newDirectory);
    QString newDataDir = QDir(instancesDir).filePath(newDirectory + "/data");
    copyDirectoryRecursive(getDirectoryFor(version), newVersionDir);
    copyDirectoryRecursive(getDataDirectoryFor(version), newDataDir);
    addVersion(newDirectory, version->versionName, version->versionCode(), newDataDir,
               version->instanceName + tr(" Copy"), true);
    return m_versions.value(newDirectory, nullptr);
}

bool VersionManager::renameVersion(VersionInfo* version, QString instanceName) {
    if (!version)
        return false;
    instanceName = instanceName.trimmed();
    if (instanceName.isEmpty())
        return false;

    QString baseName = sanitizeInstanceName(instanceName, version->versionName);
    QString storageName = baseName;
    int suffix = 2;
    QString targetRoot;
    do {
        targetRoot = QDir(instancesDir).filePath(storageName);
        if (!QFileInfo::exists(targetRoot))
            break;
        storageName = QString("%1-%2").arg(baseName).arg(suffix++);
    } while (true);
    QString targetData = QDir(targetRoot).filePath("data");
    QString oldData = getDataDirectoryFor(version);
    QString oldRoot = QFileInfo(oldData).dir().absolutePath();
    bool dedicatedStorage = QFileInfo(oldRoot).dir().absolutePath() == QFileInfo(instancesDir).absoluteFilePath();

    QDir().mkpath(instancesDir);
    bool moved = false;
    if (dedicatedStorage && QFileInfo::exists(oldRoot))
        moved = QDir().rename(oldRoot, targetRoot);
    if (!moved) {
        bool copied = copyDirectoryRecursive(oldData, targetData);
        if (!QFileInfo::exists(targetData))
            copied = QDir().mkpath(targetData);
        if (!copied) {
            QDir(targetRoot).removeRecursively();
            return false;
        }
        if (dedicatedStorage && QFileInfo::exists(oldRoot))
            QDir(oldRoot).removeRecursively();
    }

    version->instanceName = instanceName;
    version->customNamed = true;
    version->dataDirectory = targetData;
    saveVersions();
    emit version->metadataChanged();
    emit versionListChanged();
    return true;
}

QString VersionManager::getIconPathFor(VersionInfo* version) {
    if (!version)
        return QString();
    QString root = getDirectoryFor(version);
    QStringList candidates = {
        QDir(root).filePath("assets/icon.png"),
        QDir(root).filePath("assets/assets/resource_packs/vanilla/pack_icon.png"),
        QDir(root).filePath("assets/assets/resource_packs/oreui/pack_icon.png")
    };
    for (auto const& path : candidates) {
        if (QFileInfo::exists(path))
            return QUrl::fromLocalFile(path).toString();
    }
    return QString("qrc:/Resources/mcpelauncher-icon.svg");
}

bool VersionManager::openDirectory(QString path) {
    if (path.isEmpty())
        return false;
    QDir().mkpath(path);
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

bool VersionManager::checkSupport(VersionInfo* version) {
    if(!version) return false;
    return checkSupport(version->directory);
}

bool VersionManager::checkSupport(QString const& directory) {
    for (auto &&abi : SupportedAndroidAbis::getAbis()) {
        if (abi.second.compatible && QFile(getDirectoryFor(directory) + "/lib/" + QString::fromStdString(abi.first) + "/libminecraftpe.so").exists()) {
            return true;
        }
    }
    return false;
}

VersionInfo* VersionList::latestInstalledVersion() const {
    if (m_versions.empty())
        return nullptr;
    VersionInfo* latest = nullptr;
    for (auto* version : m_versions) {
        if (version && (!latest || version->versionCode() > latest->versionCode()))
            latest = version;
    }
    return latest;
}
