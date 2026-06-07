#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "versionmanager.h"
#include "apkextractiontask.h"
#include "gamelauncher.h"
#include "qmlurlutils.h"
#include "launchersettings.h"
#include "launcherapp.h"
#include "zipextractiontask.h"
#include "packimporttask.h"
#include "storagemanager.h"

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QObject>
#include <QCoreApplication>
#include <QtConcurrent>

#ifdef LAUNCHER_DISABLE_DEV_MODE
bool LauncherSettings::disableDevMode = 1;
#else
bool LauncherSettings::disableDevMode = 0;
#endif

int main(int argc, char *argv[])
{
    bool isSafeMode = getenv("SAFE_MODE") != nullptr;
#ifdef LAUNCHER_INIT_PATCH
    LAUNCHER_INIT_PATCH
#endif
    QCoreApplication::setOrganizationName("Minecraft Linux Launcher");
    QCoreApplication::setOrganizationDomain("mrarm.io");
    QCoreApplication::setApplicationName("Minecraft Linux Launcher UI");

    LauncherApp app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Minecraft Linux Launcher UI");
    parser.addPositionalArgument("file", "file or uri to open with the installed game");
    parser.addHelpOption();
    QCommandLineOption devmodeOption(QStringList() << "d" << "enable-devmode", 
        QCoreApplication::translate("main", "Developer Mode - Enable unsafe Launcher Settings"));
    parser.addOption(devmodeOption);

    QCommandLineOption verboseOption(QStringList() << "v" << "verbose", 
        QCoreApplication::translate("main", "Verbose log Qt Messages to stdout"));
    parser.addOption(verboseOption);

    parser.process(app);
    
    bool hasFileOrUri = parser.positionalArguments().count() == 1;

    if(hasFileOrUri) {
        return app.launchFile(parser.positionalArguments().at(0));
    }

    auto verbose = parser.isSet(verboseOption);

    if(!verbose) {
        // Silence console
        qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg) {});
    }

    app.setQuitOnLastWindowClosed(false);

    qmlRegisterType<VersionInfo>("io.mrarm.mcpelauncher", 1, 0, "VersionInfo");
    qmlRegisterType<VersionManager>("io.mrarm.mcpelauncher", 1, 0, "VersionManager");
    qmlRegisterType<ApkExtractionTask>("io.mrarm.mcpelauncher", 1, 0, "ApkExtractionTask");
    qmlRegisterType<GameLauncher>("io.mrarm.mcpelauncher", 1, 0, "GameLauncher");
    qmlRegisterType<LauncherSettings>("io.mrarm.mcpelauncher", 1, 0, "LauncherSettings");
    qmlRegisterSingletonType<QmlUrlUtils>("io.mrarm.mcpelauncher", 1, 0, "QmlUrlUtils", &QmlUrlUtils::createInstance);
    qmlRegisterType<ZipExtractionTask>("io.mrarm.mcpelauncher", 1, 0, "ZipExtractionTask");
    qmlRegisterType<PackImportTask>("io.mrarm.mcpelauncher", 1, 0, "PackImportTask");
    qmlRegisterType<StorageManager>("io.mrarm.mcpelauncher", 1, 0, "StorageManager");
    QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).mkpath("mcpelauncher/background_art");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("application", &app);
#ifdef LAUNCHER_VERSION_NAME
    engine.rootContext()->setContextProperty("LAUNCHER_VERSION_NAME", QVariant(LAUNCHER_VERSION_NAME));
#else
    engine.rootContext()->setContextProperty("LAUNCHER_VERSION_NAME", QVariant(""));
    QString license;
    QFile lfile(":/LICENSE");
    if(lfile.open(QIODevice::ReadOnly)) {
        license = lfile.readAll();
        lfile.close();
    }

    engine.rootContext()->setContextProperty("LAUNCHER_CHANGE_LOG", QVariant(license.replace("\n", "<br/>")));
#endif
    engine.rootContext()->setContextProperty("LAUNCHER_ENABLE_GOOGLE_PLAY_LICENCE_CHECK", QVariant(false));
#ifdef __APPLE__
    engine.rootContext()->setContextProperty("SHOW_ANGLEBACKEND", QVariant(true));
#else
    engine.rootContext()->setContextProperty("SHOW_ANGLEBACKEND", QVariant(false));
#endif
    engine.rootContext()->setContextProperty("DISABLE_DEV_MODE", QVariant(LauncherSettings::disableDevMode &= !parser.isSet(devmodeOption)));

    engine.rootContext()->setContextProperty("SAFE_MODE", QVariant(isSafeMode));

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
