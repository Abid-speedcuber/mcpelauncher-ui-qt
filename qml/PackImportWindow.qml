import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs
import QtQuick.Controls
import Qt.labs.platform
import io.mrarm.mcpelauncher 1.0
import "Components"

Window {
    id: root

    property string gameDataDir: ""

    signal importFinished

    width: 340
    height: layout.implicitHeight + layout.anchors.topMargin + layout.anchors.bottomMargin
    flags: Qt.Dialog
    title: qsTr("Import packs")
    visible: packImportTask.active
    color: "#333"

    onClosing: function (close) {
        close.accepted = false
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: 10

        MProgressBar {
            id: packImportProgressBar
            label: qsTr("Importing packs")
            indeterminate: true
            Layout.fillWidth: true
            Layout.minimumHeight: 30
        }
    }

    FileDialog {
        id: packPicker
        title: qsTr("Import worlds, resource packs, or behavior packs")
        nameFilters: [
            qsTr("Minecraft packs (*.mcworld *.mcpack *.mcaddon *.mctemplate *.zip)"),
            qsTr("All files (*)")
        ]
        fileMode: FileDialog.OpenFiles

        onAccepted: root.importUrls(packPicker.currentFiles)
    }

    PackImportTask {
        id: packImportTask
        gameDataDir: root.gameDataDir

        onProgress: function (val) {
            packImportProgressBar.indeterminate = false
            packImportProgressBar.value = val
        }

        onImportFinished: function (message) {
            packImportProgressBar.indeterminate = true
            packImportMessageDialog.text = message
            packImportMessageDialog.open()
            root.importFinished()
        }

        onError: function (err) {
            packImportProgressBar.indeterminate = true
            packImportMessageDialog.text = qsTr("The selected file could not be imported.<br/>Details:<br/>%1").arg(err)
            packImportMessageDialog.open()
        }
    }

    MessageDialog {
        id: packImportMessageDialog
        title: qsTr("Pack import")
    }

    function pickFiles() {
        packPicker.open()
    }

    function importUrls(urls) {
        if (packImportTask.active) {
            return
        }
        if (!packImportTask.setSourceUrls(urls)) {
            packImportMessageDialog.text = qsTr("Invalid file URL")
            packImportMessageDialog.open()
            return
        }
        packImportProgressBar.indeterminate = true
        packImportTask.start()
    }
}
