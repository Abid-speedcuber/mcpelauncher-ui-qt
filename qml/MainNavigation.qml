import QtQuick
import QtQuick.Layouts
import io.mrarm.mcpelauncher 1.0

ColumnLayout {
    property VersionManager versionManager
    property var launcher

    HomeScreen {
        Layout.fillWidth: true
        Layout.fillHeight: true
        versionManager: parent.versionManager
        launcher: parent.launcher
    }
}
