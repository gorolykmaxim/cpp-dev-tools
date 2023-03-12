import QtQuick
import QtQuick.Controls as QtCtrls
import cdt
import "." as Cdt

QtCtrls.Button {
  property var buttonIcon: ""
  padding: Theme.basePadding
  background: Rectangle {
    color: enabled && hovered && !pressed ? Theme.colorBgLight : "transparent"
    border.color: parent.activeFocus ? Theme.colorHighlight : "transparent"
    border.width: parent.activeFocus ? 2 : 0
    radius: Theme.baseRadius
  }
  contentItem: Cdt.Icon {
    icon: buttonIcon
    color: parent.enabled ? Theme.colorText : Theme.colorSubText
  }
  Keys.onReturnPressed: clicked()
  Keys.onEnterPressed: clicked()
}
