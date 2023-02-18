import QtQuick
import QtQuick.Controls

Button {
  property var buttonIcon: ""
  padding: basePadding
  background: Rectangle {
    color: hovered && !pressed ? colorBgLight : colorBgMedium
    border.color: parent.activeFocus ? colorHighlight : "transparent"
    border.width: parent.activeFocus ? 2 : 0
    radius: baseRadius
  }
  contentItem: IconWidget {
    icon: buttonIcon
    color: parent.enabled ? colorText : colorSubText
  }
  Keys.onReturnPressed: clicked()
  Keys.onEnterPressed: clicked()
}
