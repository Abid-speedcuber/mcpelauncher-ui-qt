import QtQuick
import QtQuick.Templates as T

T.Button {
    id: control
    padding: 10
    implicitWidth: 12 + contentItem.implicitWidth + leftPadding + rightPadding
    implicitHeight: 40
    opacity: enabled ? 1 : 0.3

    background: Rectangle {
        anchors.fill: parent
        border.color: control.down ? "#4f8db7" : (control.hovered ? "#315f7d" : "#1c3a52")
        color: control.down ? "#102b40" : "#0b1c2c"
        radius: 4
        FocusBorder {
            visible: control.visualFocus
        }
    }

    contentItem: Text {
        anchors.fill: parent
        text: control.text
        font.pointSize: 10
        color: "#fff"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
