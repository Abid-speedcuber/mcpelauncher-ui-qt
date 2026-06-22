import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.platform
import "Components"
import io.mrarm.mcpelauncher 1.0

ColumnLayout {
    id: rowLayout
    spacing: 0

    property alias headerContent: baseHeader.content
    property alias headerTitle: baseHeader.title
    property alias headerSubtitle: baseHeader.subtitle

    BaseHeader {
        id: baseHeader
        Layout.fillWidth: true
        title: qsTr("Minecraft Pocket Edition Launcher")
        subtitle: qsTr("A debloated direct fork of mrarm's MCPE Launcher")
    }
}
