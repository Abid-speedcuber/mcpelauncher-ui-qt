import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Controls
import "Components"
import io.mrarm.mcpelauncher 1.0

BaseScreen {
    id: homeScreen

    property VersionManager versionManager
    property var launcher
    property var logModel
    property var selectedVersion: null

    signal gameLogRequested()

    headerContent: RowLayout {
        anchors.fill: parent
        Text {
            text: qsTr("Unofficial Bedrock APK Launcher")
            color: "white"
            font.pointSize: 14
            font.bold: true
            Layout.leftMargin: 10
        }
        Item { Layout.fillWidth: true }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Text {
            text: qsTr("Select an imported game version:")
            color: "white"
            font.pointSize: 12
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#444444"
            radius: 5

            ListView {
                id: versionList
                anchors.fill: parent
                anchors.margins: 5
                clip: true
                model: versionManager.versions.getAll()

                delegate: Rectangle {
                    property var versionInfo: modelData
                    property bool dragHover: false

                    width: versionList.width
                    height: 60
                    color: dragHover ? "#364e63" : (homeScreen.selectedVersion === versionInfo ? "#24513a" : (index % 2 === 0 ? "#333333" : "#3a3a3a"))
                    radius: 3
                    border.color: dragHover ? "#78bde8" : (homeScreen.selectedVersion === versionInfo ? "#5fc48b" : (mouseArea.containsMouse ? "#555555" : "transparent"))

                    DropArea {
                        anchors.fill: parent
                        keys: ["text/uri-list"]
                        onEntered: {
                            dragHover = true
                            versionList.currentIndex = index
                            homeScreen.selectedVersion = versionInfo
                        }
                        onExited: dragHover = false
                        onDropped: function (drop) {
                            dragHover = false
                            versionList.currentIndex = index
                            homeScreen.selectedVersion = versionInfo
                            if (drop.hasUrls) {
                                packImportWindow.importUrls(drop.urls)
                                drop.acceptProposedAction()
                            }
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: {
                            versionList.currentIndex = index
                            homeScreen.selectedVersion = versionInfo
                            if (mouse.button === Qt.RightButton) {
                                versionContextMenu.popup()
                            }
                        }
                    }

                    Menu {
                        id: versionContextMenu
                        MenuItem {
                            text: qsTr("Import packs...")
                            onTriggered: packImportWindow.pickFiles()
                        }
                        MenuItem {
                            text: qsTr("Update APK...")
                            onTriggered: apkImportWindow.pickUpdate(versionInfo)
                        }
                        MenuItem {
                            text: qsTr("Duplicate")
                            onTriggered: {
                                var copy = versionManager.duplicateVersion(versionInfo)
                                refreshVersions(copy)
                            }
                        }
                        MenuItem {
                            text: qsTr("Manage storage...")
                            onTriggered: storageWindow.openFor(versionInfo)
                        }
                        MenuSeparator {}
                        MenuItem {
                            text: qsTr("Delete...")
                            onTriggered: deleteWindow.openFor(versionInfo)
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 2

                        Text {
                            text: modelData.versionName
                            color: "white"
                            font.pointSize: 11
                            font.bold: true
                        }

                        Text {
                            text: qsTr("Directory: %1").arg(modelData.directory)
                            color: "#cccccc"
                            font.pointSize: 9
                        }

                        Text {
                            text: qsTr("Storage: %1").arg(modelData.dataDirectory || versionManager.getDataDirectoryFor(modelData))
                            color: "#aaaaaa"
                            font.pointSize: 9
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }

                        Text {
                            text: qsTr("Architectures: %1").arg(modelData.archs.join(", "))
                            color: "#999999"
                            font.pointSize: 9
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar { }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                text: qsTr("Import APK")
                Layout.fillWidth: true
                onClicked: {
                    apkImportWindow.pickFile()
                }
            }

            Button {
                text: qsTr("Import Packs")
                Layout.fillWidth: true
                enabled: homeScreen.selectedVersion !== null
                onClicked: packImportWindow.pickFiles()
            }

            Button {
                text: qsTr("Game Log")
                Layout.fillWidth: true
                enabled: launcher.running || launcher.crashed || (logModel && logModel.count > 0)
                onClicked: homeScreen.gameLogRequested()
            }

            Button {
                text: launcher.running ? qsTr("Play Another") : qsTr("Play")
                Layout.fillWidth: true
                enabled: homeScreen.selectedVersion !== null
                onClicked: {
                    if (homeScreen.selectedVersion) {
                        var gameDir = versionManager.getDirectoryFor(homeScreen.selectedVersion)
                        var dataDir = versionManager.getDataDirectoryFor(homeScreen.selectedVersion)
                        if (logModel) {
                            if (!launcher.running)
                                logModel.clear()
                            logModel.append({ "display": "Launching " + homeScreen.selectedVersion.versionName })
                            logModel.append({ "display": "Game directory: " + gameDir })
                            logModel.append({ "display": "Data directory: " + dataDir })
                        }
                        launcher.gameDir = gameDir
                        launcher.dataDir = dataDir
                        if (logModel) {
                            logModel.append({ "display": "Starting launcher process..." })
                        }
                        launcher.start(false, "", true, "")
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("Unofficial GPLv3 fork of mcpelauncher-ui-qt. Not affiliated with Mojang or Microsoft. No game files are included.")
            color: "#b8b8b8"
            font.pointSize: 9
            wrapMode: Text.WordWrap
        }
    }

    ApkImportWindow {
        id: apkImportWindow
        versionManager: homeScreen.versionManager
        onImportFinished: {
            refreshVersions(null)
        }
    }

    PackImportWindow {
        id: packImportWindow
        gameDataDir: homeScreen.selectedVersion ? versionManager.getDataDirectoryFor(homeScreen.selectedVersion) : ""
    }

    InstanceStorageWindow {
        id: storageWindow
        versionManager: homeScreen.versionManager
    }

    Window {
        id: deleteWindow
        visible: false
        width: 360
        height: deleteLayout.implicitHeight + 24
        flags: Qt.Dialog
        title: qsTr("Delete instance")
        color: "#333"

        property var versionInfo: null

        function openFor(version) {
            versionInfo = version
            show()
            raise()
            requestActivate()
        }

        ColumnLayout {
            id: deleteLayout
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            Text {
                Layout.fillWidth: true
                color: "white"
                wrapMode: Text.WordWrap
                text: deleteWindow.versionInfo ? qsTr("Delete %1?").arg(deleteWindow.versionInfo.versionName) : ""
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("Delete APK only")
                onClicked: {
                    versionManager.deleteVersion(deleteWindow.versionInfo, false)
                    deleteWindow.close()
                    refreshVersions(null)
                }
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("Delete APK and all data")
                onClicked: {
                    versionManager.deleteVersion(deleteWindow.versionInfo, true)
                    deleteWindow.close()
                    refreshVersions(null)
                }
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("Cancel")
                onClicked: deleteWindow.close()
            }
        }
    }

    function refreshVersions(preferredVersion) {
        versionList.model = versionManager.versions.getAll()
        homeScreen.selectedVersion = preferredVersion
        versionList.currentIndex = -1
    }
}
