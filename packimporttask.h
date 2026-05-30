#ifndef PACKIMPORTTASK_H
#define PACKIMPORTTASK_H

#include <QMutex>
#include <QThread>
#include <QUrl>

class PackImportTask : public QThread {
    Q_OBJECT
    Q_PROPERTY(QStringList sources READ sources WRITE setSources)
    Q_PROPERTY(QString gameDataDir READ gameDataDir WRITE setGameDataDir)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)

    QMutex mutex;
    QStringList m_sources;
    QString m_gameDataDir;
    int m_importedWorlds = 0;
    int m_importedResourcePacks = 0;
    int m_importedBehaviorPacks = 0;

    void run() override;
    void emitActiveChanged() { emit activeChanged(); }

public:
    explicit PackImportTask(QObject *parent = nullptr);

    bool active() const { return isRunning(); }

    QStringList sources();
    void setSources(QStringList const& value);

    QString gameDataDir();
    void setGameDataDir(QString const& value);

public slots:
    bool setSourceUrls(QList<QUrl> const& urls);

signals:
    void progress(qreal progress);
    void importFinished(QString const& message);
    void error(QString const& err);
    void activeChanged();
};

#endif // PACKIMPORTTASK_H
