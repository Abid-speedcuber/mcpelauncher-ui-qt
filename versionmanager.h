#ifndef VERSIONMANAGER_H
#define VERSIONMANAGER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QHash>
#include <QStringList>
#include <QVariantList>

class CodeInfo : public QObject {
    Q_OBJECT
    Q_PROPERTY(int code MEMBER code CONSTANT)
    Q_PROPERTY(QString arch MEMBER arch CONSTANT)
public:
    CodeInfo(int code, QString arch, QObject* parent = nullptr) : code(code), arch(arch), QObject(parent) {}
    int code;
    QString arch;
};

class VersionInfo : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString directory MEMBER directory CONSTANT)
    Q_PROPERTY(QString dataDirectory MEMBER dataDirectory NOTIFY metadataChanged)
    Q_PROPERTY(QString versionName MEMBER versionName NOTIFY metadataChanged)
    Q_PROPERTY(QString instanceName MEMBER instanceName NOTIFY metadataChanged)
    Q_PROPERTY(bool customNamed MEMBER customNamed NOTIFY metadataChanged)
    Q_PROPERTY(int versionCode READ versionCode CONSTANT)
    Q_PROPERTY(QStringList archs READ archs CONSTANT)
    Q_PROPERTY(QList<CodeInfo*> codes READ getCodes CONSTANT)
public:
    QString directory;
    QString dataDirectory;
    QString versionName;
    QString instanceName;
    bool customNamed = false;
    QHash<QString, int> codes;

    VersionInfo(QObject* parent = nullptr) : QObject(parent) {}
    VersionInfo(VersionInfo const& v) : directory(v.directory), dataDirectory(v.dataDirectory), versionName(v.versionName), instanceName(v.instanceName), customNamed(v.customNamed), codes(v.codes) {}

    VersionInfo& operator=(VersionInfo const& v) {
        directory = v.directory;
        dataDirectory = v.dataDirectory;
        versionName = v.versionName;
        instanceName = v.instanceName;
        customNamed = v.customNamed;
        codes = v.codes;
        return *this;
    }

    QStringList archs() {
        QStringList archs;
        for (auto && arch : codes.keys()) {
            archs.append(arch);
        }
        return archs;
    }

    int versionCode() {
        for (auto && code : codes) {
            return code;
        }
        return -1;
    }

    QList<CodeInfo*> getCodes() {
        QList<CodeInfo*> l;
        QHash<QString, int>::const_iterator i = codes.constBegin();
        while (i != codes.constEnd()) {
            l.append(new CodeInfo(i.value(), i.key(), this));
            ++i;
        }
        return l;
    }

signals:
    void metadataChanged();
};

class VersionList : public QObject {
    Q_OBJECT
    Q_PROPERTY(int size READ size)
    Q_PROPERTY(VersionInfo* latestInstalledVersion READ latestInstalledVersion)

private:
    QMap<QString, VersionInfo*>& m_versions;

public:
    VersionList(QMap<QString, VersionInfo*>& versions) : m_versions(versions) {}

    int size() const { return m_versions.size(); }

    VersionInfo* latestInstalledVersion() const;

public slots:
    QList<QObject*> getAll() const {
        QList<QObject*> ret;
        ret.reserve(m_versions.size());
        QMap<QString, VersionInfo*>::const_iterator i = m_versions.constBegin();
        while (i != m_versions.constEnd()) {
            ret.push_back(i.value());
            ++i;
        }
        return ret;
    }

    VersionInfo* get(int versionCode) const {
        for (VersionInfo* v : m_versions) {
            if (v && v->versionCode() == versionCode)
                return v;
        }
        return nullptr;
    }
    VersionInfo* getByDirectory(QString const& directory) const {
        for (VersionInfo* v : m_versions) {
            if (v->directory == directory)
                return v;
        }
        return nullptr;
    }

    bool contains(int versionCode) const { return get(versionCode) != nullptr; }

};

class VersionManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(VersionList* versions READ versionList NOTIFY versionListChanged)

private:
    QString baseDir;
    QString instancesDir;
    QMap<QString, VersionInfo*> m_versions;
    VersionList m_versionList;

    void loadVersions();
    void saveVersions();
    bool normalizeDataDirectory(VersionInfo* version);

public:
    VersionManager();

    // This is safe in a multi-thread env, because the baseDir can not be changed
    QString const& getBaseDir() const { return baseDir; }
    QString const& getInstancesDir() const { return instancesDir; }

    QString getTempTemplate();

    QString getDirectoryFor(std::string const& versionName);

    QString createUniqueDirectoryName(QString desiredName) const;

    void addVersion(QString directory, QString versionName, int versionCode, QString dataDirectory = QString(),
                    QString instanceName = QString(), bool customNamed = false);

    VersionList* versionList() { return &m_versionList; }

public slots:
    QString getDirectoryFor(QString const& versionName);

    QString getDirectoryFor(VersionInfo* version);

    QString getDataDirectoryFor(VersionInfo* version);

    QString getWorldsDirectoryFor(VersionInfo* version);

    QString getResourcePacksDirectoryFor(VersionInfo* version);

    QString getBehaviorPacksDirectoryFor(VersionInfo* version);

    void deleteVersion(VersionInfo* version, bool removeData = false);

    void removeVersion(VersionInfo* version, QStringList abis);

    VersionInfo* duplicateVersion(VersionInfo* version);

    bool renameVersion(VersionInfo* version, QString instanceName);

    QString getIconPathFor(VersionInfo* version);

    bool openDirectory(QString path);

    bool checkSupport(QString const& versionName);

    bool checkSupport(VersionInfo *version);
    
signals:
    void versionListChanged();

};

#endif // VERSIONMANAGER_H
