import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import QtQuick.Controls
import io.mrarm.mcpelauncher 1.0

Window {
    id: window
    visible: true
    width: 800
    height: 600
    title: qsTr("Unofficial Bedrock APK Launcher")
    color: "#333333"
    property string currentGameDataDir: ""

    StackView {
        id: stackView
        anchors.fill: parent
    }

    VersionManager {
        id: versionManagerInstance
    }

    LauncherSettings {
        id: launcherSettings
    }

    ListModel {
        id: gameLog
    }

    Component {
        id: panelHome
        HomeScreen {
            versionManager: versionManagerInstance
            launcher: gameLauncher
            logModel: gameLog
            onGameLogRequested: stackView.push(panelGameLog)
        }
    }

    Component {
        id: panelGameLog
        GameLogScreen {
            launcher: gameLauncher
            logModel: gameLog
            onBackRequested: stackView.pop()
        }
    }

    Component {
        id: panelError
        ErrorScreen {
            message: qsTr("Game launch failed")
            confirm: qsTr("OK")
            onFinished: stackView.pop()
        }
    }

    GameLauncher {
        id: gameLauncher

        onLogCleared: gameLog.clear()
        onLogAppended: function (text) {
            gameLog.append({ "display": text.trim() })
        }

        onLaunchFailed: {
            stackView.push(panelError)
        }
        onStateChanged: {
            if (!running) {
                window.show()
            }
            if (crashed) {
                application.setVisibleInDock(true)
            }
        }
    }

    Connections {
        target: window
        function onClosing(close) {
            if (gameLauncher.running) {
                gameLauncher.kill()
            }
            application.quit()
        }
    }

    Component.onCompleted: {
        stackView.push(panelHome)
    }

    function getCurrentGameDataDir() {
        if (window.currentGameDataDir && window.currentGameDataDir.length > 0) {
            return window.currentGameDataDir
        }
        return launcherSettings.gameDataDir
    }
}
