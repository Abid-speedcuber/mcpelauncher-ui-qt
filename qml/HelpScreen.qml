import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Components"

BaseScreen {
    id: helpScreen
    headerTitle: qsTr("Help & About")
    headerSubtitle: qsTr("Minecraft Pocket Edition Launcher")
    headerHelpVisible: false

    signal backRequested()

    headerContent: MButton {
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        height: 36
        width: 72
        text: qsTr("Back")
        onClicked: helpScreen.backRequested()
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true

        ColumnLayout {
            width: Math.min(helpScreen.width - 48, 760)
            x: Math.max(24, (helpScreen.width - width) / 2)
            spacing: 8

            Item { Layout.preferredHeight: 10 }

            HelpSection {
                title: qsTr("Instances")
                body: qsTr("Add instance imports an x86_64 Minecraft APK into its own isolated storage. Select a tile before playing or importing packs. Right-click a tile to update its APK, rename it, duplicate it, manage its files, or delete it.")
            }
            HelpSection {
                title: qsTr("Updates and names")
                body: qsTr("Updating replaces only the selected instance's APK and keeps its worlds and packs. An unnamed instance follows the APK version name after an update; a name you chose stays unchanged. Renaming also moves that instance's storage folder.")
            }
            HelpSection {
                title: qsTr("Storage and parallel play")
                body: qsTr("Manage storage lets you inspect worlds, resource packs, and behavior packs without opening Minecraft, including copying data between instances. Each instance has a separate data directory, so multiple instances can run at the same time.")
            }
            HelpSection {
                title: qsTr("Why this fork")
                body: qsTr("This fork removes the account, download, and setup flows required by the original launcher and goes directly from your locally supplied APK to the game. Its client patches expose higher render and simulation-distance choices, report usable disk space correctly, and improve compatibility across Bedrock versions.")
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: 10
                Layout.bottomMargin: 24
                textFormat: Text.RichText
                text: qsTr("Source code, releases, and issue tracker: <a href='https://github.com/Abid-speedcuber/mcpelauncher-ui-qt'>github.com/Abid-speedcuber/mcpelauncher-ui-qt</a>")
                color: "#a9bed0"
                linkColor: "#69bce8"
                font.pointSize: 10
                wrapMode: Text.WordWrap
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                HoverHandler { cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor }
            }
        }
    }

    component HelpSection: ColumnLayout {
        required property string title
        required property string body
        Layout.fillWidth: true
        Layout.topMargin: 8
        spacing: 5

        Text {
            Layout.fillWidth: true
            text: parent.title
            color: "#ffffff"
            font.bold: true
            font.pointSize: 12
        }
        Text {
            Layout.fillWidth: true
            text: parent.body
            color: "#a9bed0"
            font.pointSize: 10
            wrapMode: Text.WordWrap
            lineHeight: 1.18
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 8
            height: 1
            color: "#17364d"
        }
    }
}
