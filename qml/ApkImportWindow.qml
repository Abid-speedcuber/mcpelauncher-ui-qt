import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs
import QtQuick.Controls
import io.mrarm.mcpelauncher 1.0
import "Components"

Window {
    property VersionManager versionManager
    signal importFinished

    id: root
    width: 320
    height: layout.implicitHeight + layout.anchors.topMargin + layout.anchors.bottomMargin
    flags: Qt.Dialog
    title: qsTr("Add instance")
    visible: apkImportHelper.extractingApk
    color: "#07111f"

    property bool allowIncompatible: false

    onClosing: function () {
        close.accepted = false
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: 10

        MProgressBar {
            id: apkExtractionProgressBar
            label: qsTr("Extracting the .apk")
            Layout.fillWidth: true
            Layout.minimumHeight: 30
        }
    }

    ApkImportHelper {
        id: apkImportHelper
        versionManager: root.versionManager
        progressBar: apkExtractionProgressBar
        allowIncompatible: root.allowIncompatible
        onFinished: root.importFinished()
    }

    Window {
        id: nameWindow
        width: 380
        height: nameLayout.implicitHeight + 28
        flags: Qt.Dialog
        title: qsTr("Name instance")
        color: "#07111f"

        ColumnLayout {
            id: nameLayout
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            Text {
                Layout.fillWidth: true
                text: qsTr("Instance name")
                color: "white"
                font.bold: true
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Leave blank to use the Minecraft version name.")
                color: "#8fa6bd"
                wrapMode: Text.WordWrap
            }
            MTextField {
                id: instanceNameField
                Layout.fillWidth: true
                placeholderText: qsTr("Optional name")
                onAccepted: continueButton.clicked()
            }
            RowLayout {
                Layout.fillWidth: true
                MButton {
                    text: qsTr("Cancel")
                    Layout.fillWidth: true
                    onClicked: nameWindow.close()
                }
                MButton {
                    id: continueButton
                    text: qsTr("Choose APK")
                    Layout.fillWidth: true
                    onClicked: {
                        var name = instanceNameField.text
                        nameWindow.close()
                        apkImportHelper.pickFile(name)
                    }
                }
            }
        }
    }

    function pickFile() {
        instanceNameField.text = ""
        nameWindow.show()
        nameWindow.raise()
        nameWindow.requestActivate()
        instanceNameField.forceActiveFocus()
    }

    function pickUpdate(versionInfo) {
        if (!versionInfo)
            return
        apkImportHelper.pickUpdate(versionInfo.directory)
    }
}
