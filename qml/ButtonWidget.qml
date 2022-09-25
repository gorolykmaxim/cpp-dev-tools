import QtQuick
import QtQuick.Controls

Button {
  topPadding: basePadding
  bottomPadding: basePadding
  leftPadding: basePadding * 4
  rightPadding: basePadding * 4
  background: Rectangle {
    color: colorBgLight
    border.color: parent.focus ? colorHighlight : colorBgDark
    border.width: parent.focus ? 2 : 1
    radius: baseRadius
  }
  contentItem: TextWidget {
    text: parent.text
  }
  Keys.onReturnPressed: clicked()
  Keys.onEnterPressed: clicked()
}
