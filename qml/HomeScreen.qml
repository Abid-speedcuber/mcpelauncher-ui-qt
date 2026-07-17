import QtQuick
import QtQuick.Window
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
    property bool hasInstances: versionGrid.count > 0

    signal gameLogRequested()

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.margins: 16
        spacing: 12

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !homeScreen.hasInstances

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 32, 360)
                spacing: 8

                MButton {
                    text: qsTr("Import APK to start")
                    Layout.fillWidth: true
                    implicitHeight: 48
                    onClicked: apkImportWindow.pickFile()
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Get an x86_64 Minecraft APK and import it here")
                    color: "#8fa6bd"
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: homeScreen.hasInstances
            color: "#07111f"
            border.color: "#142b3e"
            radius: 6

            GridView {
                id: versionGrid
                anchors.fill: parent
                anchors.margins: 12
                clip: true
                cellWidth: 184
                cellHeight: 184
                model: versionManager.versions.getAll()

                delegate: Rectangle {
                    id: instanceCard
                    property var versionInfo: modelData
                    property bool dragHover: false

                    width: 164
                    height: 164
                    color: dragHover ? "#123651" : (homeScreen.selectedVersion === versionInfo ? "#0e3b46" : "#0a1b2a")
                    radius: 6
                    border.width: 1
                    border.color: dragHover ? "#68b6e3" : (homeScreen.selectedVersion === versionInfo ? "#43b59b" : (cardMouse.containsMouse ? "#315f7d" : "#173047"))

                    DropArea {
                        anchors.fill: parent
                        keys: ["text/uri-list"]
                        onEntered: {
                            dragHover = true
                            versionGrid.currentIndex = index
                            homeScreen.selectedVersion = versionInfo
                        }
                        onExited: dragHover = false
                        onDropped: function(drop) {
                            dragHover = false
                            versionGrid.currentIndex = index
                            homeScreen.selectedVersion = versionInfo
                            if (drop.hasUrls) {
                                packImportWindow.importUrls(drop.urls)
                                drop.acceptProposedAction()
                            }
                        }
                    }

                    MouseArea {
                        id: cardMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        ToolTip.visible: containsMouse
                        ToolTip.text: versionManager.getDataDirectoryFor(versionInfo)
                        ToolTip.delay: 500
                        ToolTip.timeout: 10000
                        onClicked: function(mouse) {
                            versionGrid.currentIndex = index
                            homeScreen.selectedVersion = versionInfo
                            if (mouse.button === Qt.RightButton)
                                instanceMenu.popup()
                        }
                    }

                    Menu {
                        id: instanceMenu
                        MenuItem { text: qsTr("Import packs..."); onTriggered: packImportWindow.pickFiles() }
                        MenuItem { text: qsTr("Update APK..."); onTriggered: apkImportWindow.pickUpdate(versionInfo) }
                        MenuItem { text: qsTr("Rename..."); onTriggered: renameWindow.openFor(versionInfo) }
                        MenuItem {
                            text: qsTr("Duplicate")
                            onTriggered: refreshVersions(versionManager.duplicateVersion(versionInfo))
                        }
                        MenuItem { text: qsTr("Manage storage..."); onTriggered: storageWindow.openFor(versionInfo) }
                        MenuSeparator {}
                        MenuItem { text: qsTr("Delete..."); onTriggered: deleteWindow.openFor(versionInfo) }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 5

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: Math.min(92, instanceCard.width * 0.48)
                            Layout.preferredHeight: width
                            color: "#050c16"
                            radius: 5
                            clip: true

                            Image {
                                anchors.fill: parent
                                anchors.margins: 4
                                source: versionManager.getIconPathFor(versionInfo)
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                mipmap: true
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: modelData.instanceName
                            color: "white"
                            font.pointSize: 11
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Version %1").arg(modelData.versionName)
                            color: "#a9bed0"
                            font.pointSize: 9
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: versionManager.getDataDirectoryFor(modelData)
                            color: "#708ba2"
                            font.pointSize: 8
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideMiddle
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {}
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            visible: homeScreen.hasInstances

            MButton {
                text: qsTr("Add instance")
                Layout.fillWidth: true
                onClicked: apkImportWindow.pickFile()
            }
            MButton {
                text: qsTr("Import packs")
                Layout.fillWidth: true
                enabled: homeScreen.selectedVersion !== null
                onClicked: packImportWindow.pickFiles()
            }
            MButton {
                text: qsTr("Game log")
                Layout.fillWidth: true
                enabled: launcher.running || launcher.crashed || (logModel && logModel.count > 0)
                onClicked: homeScreen.gameLogRequested()
            }
            MButton {
                text: launcher.running ? qsTr("Play another") : qsTr("Play")
                Layout.fillWidth: true
                enabled: homeScreen.selectedVersion !== null
                onClicked: launchSelected()
            }
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("Unofficial GPLv3 fork. Not affiliated with Mojang or Microsoft. No game files are included.")
            color: "#708ba2"
            font.pointSize: 9
            wrapMode: Text.WordWrap
        }
    }

    ApkImportWindow {
        id: apkImportWindow
        versionManager: homeScreen.versionManager
        onImportFinished: refreshVersions(null)
    }

    PackImportWindow {
        id: packImportWindow
        gameDataDir: homeScreen.selectedVersion ? versionManager.getDataDirectoryFor(homeScreen.selectedVersion) : ""
    }

    InstanceStorageWindow { id: storageWindow; versionManager: homeScreen.versionManager }

    Window {
        id: renameWindow
        width: 380
        height: renameLayout.implicitHeight + 28
        flags: Qt.Dialog
        title: qsTr("Rename instance")
        color: "#07111f"
        property var versionInfo: null

        function openFor(version) {
            versionInfo = version
            renameField.text = version ? version.instanceName : ""
            show()
            raise()
            requestActivate()
            renameField.forceActiveFocus()
            renameField.selectAll()
        }

        ColumnLayout {
            id: renameLayout
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10
            Text {
                Layout.fillWidth: true
                text: qsTr("Renaming also moves this instance's storage folder.")
                color: "#a9bed0"
                wrapMode: Text.WordWrap
            }
            MTextField {
                id: renameField
                Layout.fillWidth: true
                placeholderText: qsTr("Instance name")
                onAccepted: renameButton.clicked()
            }
            RowLayout {
                Layout.fillWidth: true
                MButton { text: qsTr("Cancel"); Layout.fillWidth: true; onClicked: renameWindow.close() }
                MButton {
                    id: renameButton
                    text: qsTr("Rename")
                    Layout.fillWidth: true
                    enabled: renameField.text.trim().length > 0
                    onClicked: {
                        if (versionManager.renameVersion(renameWindow.versionInfo, renameField.text)) {
                            var renamed = renameWindow.versionInfo
                            renameWindow.close()
                            refreshVersions(renamed)
                        }
                    }
                }
            }
        }
    }

    Window {
        id: deleteWindow
        width: 380
        height: deleteLayout.implicitHeight + 28
        flags: Qt.Dialog
        title: qsTr("Delete instance")
        color: "#07111f"
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
            anchors.margins: 14
            spacing: 10
            Text {
                Layout.fillWidth: true
                color: "white"
                wrapMode: Text.WordWrap
                text: deleteWindow.versionInfo ? qsTr("Delete %1?").arg(deleteWindow.versionInfo.instanceName) : ""
            }
            MButton {
                Layout.fillWidth: true
                text: qsTr("Delete APK only")
                onClicked: { versionManager.deleteVersion(deleteWindow.versionInfo, false); deleteWindow.close(); refreshVersions(null) }
            }
            MButton {
                Layout.fillWidth: true
                text: qsTr("Delete APK and all data")
                onClicked: { versionManager.deleteVersion(deleteWindow.versionInfo, true); deleteWindow.close(); refreshVersions(null) }
            }
            MButton { Layout.fillWidth: true; text: qsTr("Cancel"); onClicked: deleteWindow.close() }
        }
    }

    function launchSelected() {
        if (!homeScreen.selectedVersion)
            return
        var gameDir = versionManager.getDirectoryFor(homeScreen.selectedVersion)
        var dataDir = versionManager.getDataDirectoryFor(homeScreen.selectedVersion)
        if (logModel) {
            if (!launcher.running)
                logModel.clear()
            logModel.append({"display": "Launching " + homeScreen.selectedVersion.instanceName})
            logModel.append({"display": "Game directory: " + gameDir})
            logModel.append({"display": "Data directory: " + dataDir})
        }
        launcher.gameDir = gameDir
        launcher.dataDir = dataDir
        launcher.start(false, "", true, "")
    }

    function refreshVersions(preferredVersion) {
        versionGrid.model = versionManager.versions.getAll()
        homeScreen.selectedVersion = preferredVersion
        versionGrid.currentIndex = -1
    }
}
