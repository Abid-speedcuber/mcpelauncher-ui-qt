import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs
import QtQuick.Controls
import io.mrarm.mcpelauncher 1.0
import "Components"

Window {
    id: root

    property VersionManager versionManager
    property var versionInfo: null
    property var entries: []
    property var targetVersions: []
    property int selectedEntry: -1

    width: 620
    height: 460
    flags: Qt.Dialog
    title: versionInfo ? qsTr("Storage: %1").arg(versionInfo.versionName) : qsTr("Storage")
    color: "#07111f"

    StorageManager {
        id: storageManager
        versionManager: root.versionManager
    }

    MessageDialog {
        id: messageDialog
        title: qsTr("Storage")
    }

    MessageDialog {
        id: deleteDialog
        title: qsTr("Delete item")
        text: root.selectedEntry >= 0 ? qsTr("Delete %1?").arg(root.entries[root.selectedEntry].name) : ""
        buttons: MessageDialog.Yes | MessageDialog.No
        onAccepted: {
            if (root.selectedEntry < 0)
                return
            storageManager.deleteEntry(root.versionInfo, categoryBox.currentValue, root.entries[root.selectedEntry].name)
            root.refresh()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            MComboBox {
                id: categoryBox
                Layout.preferredWidth: 180
                textRole: "label"
                valueRole: "value"
                model: [
                    { "label": qsTr("Worlds"), "value": "worlds" },
                    { "label": qsTr("Resource Packs"), "value": "resource_packs" },
                    { "label": qsTr("Behavior Packs"), "value": "behavior_packs" },
                    { "label": qsTr("Data Root"), "value": "root" }
                ]
                onActivated: root.refresh()
            }

            MButton {
                text: qsTr("Refresh")
                onClicked: root.refresh()
            }

            MButton {
                text: qsTr("Show in Files")
                onClicked: storageManager.openCategory(root.versionInfo, categoryBox.currentValue)

                ToolTip.visible: hovered
                ToolTip.text: qsTr("Open this location in the default file manager")
                ToolTip.delay: 500
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#081725"
            border.color: "#24465e"
            radius: 3

            ListView {
                id: entryList
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                model: root.entries
                currentIndex: root.selectedEntry

                delegate: Rectangle {
                    id: entryDelegate
                    required property var modelData
                    width: entryList.width
                    height: 38
                    color: index === root.selectedEntry ? "#0e3b46" : (index % 2 === 0 ? "#0a1b2a" : "#0c2030")
                    border.color: mouseArea.containsMouse ? "#315f7d" : "transparent"

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.selectedEntry = index
                        onDoubleClicked: {
                            root.selectedEntry = index
                            if (entryDelegate.modelData.isDir)
                                storageManager.openEntry(root.versionInfo, categoryBox.currentValue, entryDelegate.modelData.name)
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 8

                        Text {
                            Layout.fillWidth: true
                            text: entryDelegate.modelData.name
                            color: "white"
                            elide: Text.ElideRight
                        }

                        Text {
                            text: entryDelegate.modelData.isDir ? qsTr("Folder") : qsTr("File")
                            color: "#8fa6bd"
                            font.pointSize: 9
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {}
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            MComboBox {
                id: targetBox
                Layout.fillWidth: true
                textRole: "instanceName"
                model: root.targetVersions
            }

            MButton {
                text: qsTr("Copy To")
                enabled: root.selectedEntry >= 0 && targetBox.currentIndex >= 0
                onClicked: {
                    var target = root.targetVersions[targetBox.currentIndex]
                    if (!target || target === root.versionInfo) {
                        messageDialog.text = qsTr("Choose a different target instance.")
                        messageDialog.open()
                        return
                    }
                    if (!storageManager.copyEntry(root.versionInfo, target, categoryBox.currentValue, root.entries[root.selectedEntry].name)) {
                        messageDialog.text = qsTr("Copy failed.")
                        messageDialog.open()
                    }
                }
            }

            MButton {
                text: qsTr("Delete")
                enabled: root.selectedEntry >= 0
                onClicked: deleteDialog.open()
            }
        }
    }

    function openFor(version) {
        versionInfo = version
        selectedEntry = -1
        targetVersions = versionManager.versions.getAll()
        refresh()
        show()
        raise()
        requestActivate()
    }

    function refresh() {
        selectedEntry = -1
        entries = versionInfo ? storageManager.listEntries(versionInfo, categoryBox.currentValue) : []
    }
}
