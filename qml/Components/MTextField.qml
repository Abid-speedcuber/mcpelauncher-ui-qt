import QtQuick
import QtQuick.Controls

TextField {
    id: control
    padding: 8
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    implicitHeight: 35
    verticalAlignment: TextInput.AlignVCenter
    font.pointSize: 10
    selectByMouse: true
    selectionColor: "#286a96"
    color: "#fff"
    placeholderTextColor: "#6f879d"
    opacity: control.enabled ? 1.0 : 0.3

    background: Rectangle {
        border.color: control.hovered ? "#315f7d" : "#1c3a52"
        color: "#081725"
        radius: 4

        FocusBorder {
            visible: control.focus ? control.focusReason == Qt.TabFocusReason || control.focusReason == Qt.BacktabFocusReason : false
        }
    }
}
