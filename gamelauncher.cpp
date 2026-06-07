#include "gamelauncher.h"
#include "EnvPathUtil.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcessEnvironment>
#include "supportedandroidabis.h"
#include <sstream>

GameLauncher::GameLauncher(QObject *parent) : QObject(parent) {
}

std::string GameLauncher::findLauncher(std::string name) {
    std::string path;

#ifdef GAME_LAUNCHER_PATH
    if (EnvPathUtil::findInPath(name, path, GAME_LAUNCHER_PATH, EnvPathUtil::getAppDir().c_str()))
        return path;
#endif
    if (EnvPathUtil::findInPath(name, path))
        return path;
    if (name == "mcpelauncher-client" && QFileInfo::exists("/usr/local/bin/mcpelauncher-client"))
        return "/usr/local/bin/mcpelauncher-client";
    return std::string();
}

void GameLauncher::start(bool disableGameLog, QString arch, bool hasVerifiedLicense, QString filepath) {
    m_disableGameLog = disableGameLog;
    QProcess* process = new QProcess(this);
    QStringList args;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    
    if (m_gameDir.length() > 0) {
        args.append("-dg");
        args.append(m_gameDir);
    }
    if (m_dataDir.length() > 0) {
        args.append("-dd");
        args.append(m_dataDir);
    }
    if (filepath.length() > 0) {
        args.append("--import-file-path");
        args.append(filepath);
    }
    if (!hasVerifiedLicense) {
        args.append("--free-only");
    }
    env.insert("PATH", "/usr/local/bin:/usr/bin:/bin:" + env.value("PATH"));
    if (env.value("BROWSER").isEmpty()) {
        env.insert("BROWSER", "xdg-open");
    }

    process->setProcessEnvironment(env);
    process->setProcessChannelMode(QProcess::MergedChannels);
    if (m_disableGameLog) {
        #ifdef _WIN32
            process->setStandardOutputFile("nul");
        #else
            process->setStandardOutputFile("/dev/null");
        #endif
    }
    if (processes.isEmpty())
        emit logCleared();
    emit logAppended(tr("Game directory: %1\n").arg(m_gameDir));
    emit logAppended(tr("Data directory: %1\n").arg(m_dataDir));
    emit logAppended(tr("PATH: %1\n").arg(env.value("PATH")));
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &GameLauncher::handleFinished);
    connect(process, &QProcess::errorOccurred, this, &GameLauncher::handleError);
    if (!m_disableGameLog && m_gamelogopen)
        connect(process, &QProcess::readyReadStandardOutput, this, &GameLauncher::handleStdOutAvailable);
    m_crashed = false;
    std::stringstream errormsg;
    auto abis = SupportedAndroidAbis::getAbis();
    std::string launcherpath;
    auto _arch = arch.toStdString();

    auto getCommandLine = [&](const QString& executable) {
        std::ostringstream commandline;
        commandline << "\"" << executable.toStdString() << "\"";
        for(auto&& v : args) {
            commandline << " \"" << v.toStdString() << "\"";
        }
        return commandline.str();
    };

    for (auto&& abi : abis) {
        auto libPath = m_gameDir + "/lib/" + QString::fromStdString(abi.first) + "/libminecraftpe.so";
        emit logAppended(tr("Checking ABI %1: %2\n").arg(QString::fromStdString(abi.first), QFile(libPath).exists() ? tr("found") : tr("missing")));
        if((_arch.empty() || _arch == abi.first) && QFile(libPath).exists()) {
            if(!(launcherpath = findLauncher(abi.second.launchername)).empty()) {
                auto executable = QString::fromStdString(launcherpath);
                emit logAppended(tr("Using launcher: %1\n").arg(executable));
                process->start(executable, args);
                processes.append(process);
                emit stateChanged();
                emit logAppended(QString::fromStdString(getCommandLine(executable)) + "\n");
                return;
            } else {
                errormsg << tr("Could not find the gamelauncher for Minecraft (%1)\nPlease add the launcher '%2' to your 'PATH' (environmentvariable) and restart the launcher\n").arg(QString::fromStdString(abi.first)).arg(QString::fromStdString(abi.second.launchername)).toStdString();
            }
        }
    }
    if(errormsg.width() == 0) {
        errormsg << "Game not found\n";
    }
    process->deleteLater();
    m_crashed = true;
    emit stateChanged();
    emit logAppended(QString::fromStdString(getCommandLine("mcpelauncher-client")) + "\n");
    emit logAppended(QString::fromStdString(errormsg.str()));
    emit launchFailed();
}

void GameLauncher::startFile(QString file) {
    fileprocess.reset(new QProcess);
    QStringList args;
    args.append(file);
    fileprocess->setProcessChannelMode(QProcess::MergedChannels);
    logAttached();
    connect(fileprocess.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this](int exitCode, QProcess::ExitStatus exitStatus) {
        emit fileStarted(exitCode == 0);
    });
    connect(fileprocess.data(), &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
        emit fileStarted(false);
    });
    connect(fileprocess.data(), &QProcess::readyReadStandardOutput, [this]() {
        emit logAppended(QString::fromUtf8(fileprocess->readAllStandardOutput()));
    });

    std::string launcherpath;
    if(!(launcherpath = findLauncher("mcpelauncher-client")).empty()) {
        fileprocess->start(QString::fromStdString(launcherpath), args);
        return;
    } else {
        emit fileStarted(false);
    }
}

void GameLauncher::handleStdOutAvailable() {
    auto proc = qobject_cast<QProcess*>(sender());
    if (!proc && !processes.isEmpty())
        proc = processes.last();
    if (proc)
        emit logAppended(QString::fromUtf8(proc->readAllStandardOutput()));
}

void GameLauncher::handleFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    auto proc = qobject_cast<QProcess*>(sender());
    if(!m_disableGameLog && proc && proc->bytesAvailable()) {
        handleStdOutAvailable();
    }
    QString msg;
    switch (exitCode)
    {
    case 51: // Failed to load Minecraft lib
        msg = tr("Incompatible Minecraft installation. Please import a compatible APK and try again.");
        emit corruptedInstall();
        break;
    case 127: // Failed to load launcher dependencies (GNU/Linux)
        msg = tr("Missing launcher dependencies, please install all missing libraries in their right version");
        emit launchFailed();
        break;
    default:
        m_crashed = exitCode != 0;
        if (m_crashed) {
            msg = tr("Process exited with unexpected exit code: %1\n").arg(exitCode);
            logAttached();
        } else {
            msg = tr("Process exited normally\n");
        }
        break;
    }
    if (proc) {
        processes.removeAll(proc);
        proc->deleteLater();
    }
    if (!m_disableGameLog)
        emit logAppended("\n" + msg);
    emit stateChanged();
}

void GameLauncher::handleError(QProcess::ProcessError error) {
    m_crashed = true;
    auto proc = qobject_cast<QProcess*>(sender());
    if (proc) {
        processes.removeAll(proc);
        proc->deleteLater();
    }
    emit logAppended(tr("Launcher process error %1: %2\n").arg(error).arg(proc ? proc->errorString() : QString()));
    emit stateChanged();
    launchFailed();
}

void GameLauncher::kill() {
    auto runningProcesses = processes;
    processes.clear();
    for (auto* process : runningProcesses) {
        disconnect(process, nullptr, this, nullptr);
        process->kill();
        process->waitForFinished();
        process->deleteLater();
    }
    emit stateChanged();
}

void GameLauncher::logAttached() {
    if(!m_disableGameLog) {
        m_gamelogopen = true;
        for (auto* process : processes) {
            connect(process, &QProcess::readyReadStandardOutput, this, &GameLauncher::handleStdOutAvailable, Qt::UniqueConnection);
        }
    }
}

void GameLauncher::logDetached() {
    if(!m_disableGameLog) {
        m_gamelogopen = false;
        for (auto* process : processes) {
            disconnect(process, &QProcess::readyReadStandardOutput, this, &GameLauncher::handleStdOutAvailable);
        }
    }
}
