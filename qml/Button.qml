import QtQuick
import QtQuick.Controls
import cdt
import "." as Cdt

Button {
  topPadding: Theme.basePadding
  bottomPadding: Theme.basePadding
  leftPadding: Theme.basePadding * 4
  rightPadding: Theme.basePadding * 4
  background: Rectangle {
    color: hovered && !pressed ? Theme.colorBgBright : Theme.colorBgLight
    border.color: parent.activeFocus ? Theme.colorHighlight : Theme.colorBgDark
    border.width: parent.activeFocus ? 2 : 1
    radius: Theme.baseRadius
  }
  contentItem: Cdt.Text {
    text: parent.text
  }
  Keys.onReturnPressed: clicked()
  Keys.onEnterPressed: clicked()
}
