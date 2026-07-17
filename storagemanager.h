#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include <QObject>
#include <QVariantList>
#include "versionmanager.h"

class StorageManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(VersionManager* versionManager READ versionManager WRITE setVersionManager)

    VersionManager* m_versionManager = nullptr;

    QString categoryDirectory(VersionInfo* version, QString const& category) const;

public:
    explicit StorageManager(QObject* parent = nullptr) : QObject(parent) {}

    VersionManager* versionManager() const { return m_versionManager; }
    void setVersionManager(VersionManager* versionManager) { m_versionManager = versionManager; }

public slots:
    QVariantList listEntries(VersionInfo* version, QString category) const;
    bool deleteEntry(VersionInfo* version, QString category, QString name);
    bool copyEntry(VersionInfo* fromVersion, VersionInfo* toVersion, QString category, QString name);
    bool openCategory(VersionInfo* version, QString category) const;
    bool openEntry(VersionInfo* version, QString category, QString name) const;
};

#endif
