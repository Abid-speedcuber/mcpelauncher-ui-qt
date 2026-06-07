#include "versionmanager.h"

#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>

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
            m_versions[group] = ver;
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
                for (auto &&abi : SupportedAndroidAbis::getAbis()) {
                    if (QFile(getDirectoryFor(ver->directory) + "/lib/" + QString::fromStdString(abi.first) + "/libminecraftpe.so").exists()) {
                        ver->codes[QString::fromStdString(abi.first)] = versionCode;
                    }
                }
            }
        }
        
        settings.endGroup();
    }
}

void VersionManager::saveVersions() {
    QSettings settings(QDir(baseDir).filePath("versions.ini"), QSettings::IniFormat);
    settings.clear();
    for (auto const& ver : m_versions) {
        if (!ver)
            continue;
        settings.beginGroup(ver->directory);
        int i = 0;
        settings.setValue("versionName", ver->versionName);
        settings.setValue("versionCode", ver->versionCode());
        settings.setValue("dataDirectory", ver->dataDirectory);
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

void VersionManager::addVersion(QString directory, QString versionName, int versionCode, QString dataDirectory) {
    auto& ver = m_versions[directory];
    if (ver == nullptr)
        ver = new VersionInfo(this);
    ver->directory = directory;
    if (dataDirectory.isEmpty() && ver->dataDirectory.isEmpty())
        dataDirectory = QDir(instancesDir).filePath(directory + "/data");
    if (!dataDirectory.isEmpty())
        ver->dataDirectory = dataDirectory;
    ver->versionName = versionName;
    ver->codes.clear();
    for (auto &&abi : SupportedAndroidAbis::getAbis()) {
        auto && it = ver->codes.constFind(QString::fromStdString(abi.first));
        if (it == ver->codes.constEnd() && QFile(getDirectoryFor(ver->directory) + "/lib/" + QString::fromStdString(abi.first) + "/libminecraftpe.so").exists()) {
            ver->codes[QString::fromStdString(abi.first)] = versionCode;
        }
    }
    saveVersions();
    emit versionListChanged();
}

QString VersionManager::getDataDirectoryFor(VersionInfo* version) {
    if (version == nullptr)
        return QString();
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

static void copyDirectoryRecursive(QString const& from, QString const& to) {
    QDir source(from);
    if (!source.exists())
        return;
    QDir().mkpath(to);
    for (auto const& entry : source.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
        QString target = QDir(to).filePath(entry.fileName());
        if (entry.isDir()) {
            copyDirectoryRecursive(entry.absoluteFilePath(), target);
        } else if (entry.isFile()) {
            QFile::remove(target);
            QFile::copy(entry.absoluteFilePath(), target);
        }
    }
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
    addVersion(newDirectory, version->versionName + tr(" Copy"), version->versionCode(), newDataDir);
    return m_versions.value(newDirectory, nullptr);
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
