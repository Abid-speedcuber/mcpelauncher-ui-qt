import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs
import QtQuick.Controls
import Qt.labs.platform
import io.mrarm.mcpelauncher 1.0
import "Components"

Item {
    signal started
    signal finished
    signal error

    property VersionManager versionManager
    property bool extractingApk: false
    property var progressBar: null
    property alias task: apkExtractionTask
    property bool allowIncompatible: false
    property bool trialMode: launcherSettings.trialMode
    property string pendingTargetDirectory: ""
    property string pendingInstanceName: ""
    property bool pendingCustomNamed: false

    id: root

    FileDialog {
        id: apkPicker
        title: "Please pick a legally obtained Android APK"
        nameFilters: ["Android package files (*.apk *.zip)", "All files (*)"]
        fileMode: FileDialog.OpenFiles

        onAccepted: {
            if (!apkExtractionTask.setSourceUrls(apkPicker.currentFiles)) {
                apkExtractionMessageDialog.text = "Invalid file URL"
                apkExtractionMessageDialog.open()
                return
            }
            console.log("Extracting " + apkExtractionTask.sources.join(','))
            apkExtractionTask.targetDirectory = root.pendingTargetDirectory
            apkExtractionTask.instanceName = root.pendingInstanceName
            apkExtractionTask.customNamed = root.pendingCustomNamed
            root.pendingTargetDirectory = ""
            root.pendingInstanceName = ""
            root.pendingCustomNamed = false
            extractingApk = true
            root.started()
            apkExtractionTask.start()
        }
    }

    ApkExtractionTask {
        id: apkExtractionTask
        versionManager: root.versionManager

        onProgress: function (val) {
            root.progressBar.indeterminate = false
            root.progressBar.value = val
        }

        onFinished: function () {
            root.finished()
            extractingApk = false
        }

        onError: function (err) {
            extractingApk = false
            apkExtractionMessageDialog.text = qsTr("The specified file is not compatible with the launcher.<br/>Details:<br/>%1").arg(err)
            apkExtractionMessageDialog.open()
            root.error()
        }

        allowIncompatible: root.allowIncompatible
        allowedPackages: {
            var packages = ["com.mojang.minecrafttrialpe", "com.mojang.minecraftedu"]
            if (!root.trialMode) {
                packages.push("com.mojang.minecraftpe")
            }
            return packages
        }
    }

    MessageDialog {
        id: apkExtractionMessageDialog
        title: "Apk extraction"
    }

    function pickFile(instanceName) {
        pendingTargetDirectory = ""
        pendingInstanceName = instanceName ? instanceName.trim() : ""
        pendingCustomNamed = pendingInstanceName.length > 0
        apkPicker.open()
    }

    function pickUpdate(targetDirectory) {
        pendingTargetDirectory = targetDirectory
        pendingInstanceName = ""
        pendingCustomNamed = false
        apkPicker.open()
    }
}
