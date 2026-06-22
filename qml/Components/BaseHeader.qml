import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: root
    property string title
    property string subtitle: ""
    property alias content: actions.data
    property bool helpVisible: true
    signal helpRequested()

    color: "transparent"
    Layout.fillWidth: true
    Layout.minimumHeight: 76
    Layout.preferredHeight: 76
    z: 2

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: shadow.top
        color: "#0d2234"
        border.color: "#173b55"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 14
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    Layout.fillWidth: true
                    text: root.title
                    color: "#ffffff"
                    font.bold: true
                    font.pointSize: 13
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    visible: root.subtitle.length > 0
                    text: root.subtitle
                    font.pointSize: 8
                    color: "#91adc4"
                    elide: Text.ElideRight
                }
            }

            Item {
                id: actions
                Layout.preferredWidth: children.length > 0 ? 160 : 0
                Layout.fillHeight: true
            }

            MButton {
                visible: root.helpVisible
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                text: "?"
                font.bold: true
                font.pointSize: 12
                onClicked: root.helpRequested()

                ToolTip.visible: hovered
                ToolTip.text: qsTr("Help and launcher information")
                ToolTip.delay: 500
            }
        }
    }

    Rectangle {
        id: shadow
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 6
        color: "#02070d"
        opacity: 0.72
    }
}
