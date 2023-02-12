import QtQuick
import QtQuick.Controls

Button {
  topPadding: basePadding
  bottomPadding: basePadding
  leftPadding: basePadding * 4
  rightPadding: basePadding * 4
  background: Rectangle {
    color: pressed ? colorBgHighlight : (hovered ? colorBgBright : colorBgLight)
    border.color: parent.activeFocus ? colorHighlight : colorBgDark
    border.width: parent.activeFocus ? 2 : 1
    radius: baseRadius
  }
  contentItem: TextWidget {
    text: parent.text
  }
  Keys.onReturnPressed: clicked()
  Keys.onEnterPressed: clicked()
}
