#include "storagemanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVariantMap>

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

static QString uniquePath(QString parentPath, QString name) {
    QDir parent(parentPath);
    QString candidate = parent.filePath(name);
    QFileInfo info(name);
    QString base = info.completeBaseName();
    QString suffix = info.suffix();
    if (base.isEmpty())
        base = name;
    int counter = 2;
    while (QFileInfo::exists(candidate)) {
        QString fileName = suffix.isEmpty()
            ? QString("%1-%2").arg(base).arg(counter++)
            : QString("%1-%2.%3").arg(base).arg(counter++).arg(suffix);
        candidate = parent.filePath(fileName);
    }
    return candidate;
}

QString StorageManager::categoryDirectory(VersionInfo* version, QString const& category) const {
    if (!m_versionManager || !version)
        return QString();
    if (category == "worlds")
        return m_versionManager->getWorldsDirectoryFor(version);
    if (category == "resource_packs")
        return m_versionManager->getResourcePacksDirectoryFor(version);
    if (category == "behavior_packs")
        return m_versionManager->getBehaviorPacksDirectoryFor(version);
    return m_versionManager->getDataDirectoryFor(version);
}

QVariantList StorageManager::listEntries(VersionInfo* version, QString category) const {
    QVariantList entries;
    QString path = categoryDirectory(version, category);
    QDir dir(path);
    if (!dir.exists())
        return entries;

    for (auto const& entry : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries, QDir::DirsFirst | QDir::Name)) {
        QVariantMap item;
        item["name"] = entry.fileName();
        item["path"] = entry.absoluteFilePath();
        item["isDir"] = entry.isDir();
        item["size"] = entry.isFile() ? entry.size() : 0;
        entries.append(item);
    }
    return entries;
}

bool StorageManager::deleteEntry(VersionInfo* version, QString category, QString name) {
    QString parent = categoryDirectory(version, category);
    if (parent.isEmpty() || name.isEmpty())
        return false;
    QFileInfo target(QDir(parent).filePath(name));
    QString parentPath = QFileInfo(parent).absoluteFilePath();
    if (!target.absoluteFilePath().startsWith(parentPath + "/"))
        return false;
    if (target.isDir())
        return QDir(target.absoluteFilePath()).removeRecursively();
    return QFile::remove(target.absoluteFilePath());
}

bool StorageManager::copyEntry(VersionInfo* fromVersion, VersionInfo* toVersion, QString category, QString name) {
    QString fromParent = categoryDirectory(fromVersion, category);
    QString toParent = categoryDirectory(toVersion, category);
    if (fromParent.isEmpty() || toParent.isEmpty() || name.isEmpty())
        return false;
    QFileInfo source(QDir(fromParent).filePath(name));
    QString fromParentPath = QFileInfo(fromParent).absoluteFilePath();
    if (!source.exists() || !source.absoluteFilePath().startsWith(fromParentPath + "/"))
        return false;
    QDir().mkpath(toParent);
    QString target = uniquePath(toParent, source.fileName());
    if (source.isDir()) {
        copyDirectoryRecursive(source.absoluteFilePath(), target);
        return QFileInfo::exists(target);
    }
    return QFile::copy(source.absoluteFilePath(), target);
}

bool StorageManager::openCategory(VersionInfo* version, QString category) const {
    if (!m_versionManager)
        return false;
    return m_versionManager->openDirectory(categoryDirectory(version, category));
}

bool StorageManager::openEntry(VersionInfo* version, QString category, QString name) const {
    if (!m_versionManager || name.isEmpty())
        return false;
    QString parent = categoryDirectory(version, category);
    QFileInfo target(QDir(parent).filePath(name));
    QString parentPath = QFileInfo(parent).absoluteFilePath();
    if (!target.isDir() || !target.absoluteFilePath().startsWith(parentPath + "/"))
        return false;
    return m_versionManager->openDirectory(target.absoluteFilePath());
}
