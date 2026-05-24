#include "launcherapp.h"
#include <QIcon>
#include <QFileOpenEvent>
#include <QDebug>
#include "versionmanager.h"
#include "gamelauncher.h"

LauncherApp::LauncherApp(int &argc, char **argv) : QApplication(argc, argv) {
    auto appdir = getenv("APPDIR");
    if(appdir != nullptr)
        setWindowIcon(QIcon(QString::fromUtf8(appdir) + "/mcpelauncher-ui-qt.png"));
}

bool LauncherApp::event(QEvent *event) {
    if (event->type() == QEvent::Close) {
        AppCloseEvent qmlEvent;
        emit closing(&qmlEvent);
        if (!qmlEvent.isAccepted()) {
            event->setAccepted(false);
            return true;
        }
    } else if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        auto url = openEvent->url();
        qDebug() << "Open Url " << url;
        if(url.isLocalFile()) {
            launchFile(url.toLocalFile(), false);
        } else if(url.isValid()) {
            launchFile(url.toString(), false);
        } else {
            launchFile(openEvent->file(), false);
        }
    }
    return QApplication::event(event);
}

int LauncherApp::launchFile(QString filePath, bool startEventLoop) {
    VersionManager vmanager;

    GameLauncher launcher;
    launcher.logAttached();
    QObject::connect(&launcher, &GameLauncher::logAppended, [](QString str) {
        printf("%s", str.toStdString().data());
    });
    int exitCode = -1;
    bool exited = false;
    auto shouldExit = [&](int code) {
        exitCode = code;
        exited = true;
        this->exit(code);
    };
    QObject::connect(&launcher, &GameLauncher::stateChanged, [&]() {
        if(!launcher.running() && startEventLoop) {
            this->exit(launcher.crashed() ? 1 : 0);
        }
    });
    QObject::connect(&launcher, &GameLauncher::launchFailed, [&]() {
        if(startEventLoop) {
            shouldExit(1);
        }
    });
    
    // Use default arch (empty string means let the system decide)
    QString arch = "";
    
    QObject::connect(&launcher, &GameLauncher::fileStarted, [&](bool success) {
        if(success) {
            if(startEventLoop) {
                shouldExit(success ? 0 : 1);
            }
        } else {
            launcher.start(false, arch, true, filePath);
        }
    });
    
    auto versionInfo = vmanager.versionList()->latestInstalledVersion();
    if(versionInfo != nullptr) {
        launcher.setGameDir(vmanager.getDirectoryFor(versionInfo));
    } else {
        printf("No game versions found!\n");
        return 1;
    }
    
    if(filePath.length() > 0) {
        launcher.startFile(filePath);
    } else {
        launcher.start(false, arch, true);
    }
    return exited ? exitCode : startEventLoop ? this->exec() : 0;
}


#ifndef __APPLE__
void LauncherApp::setVisibleInDock(bool) {
    // stub
}
#endif
