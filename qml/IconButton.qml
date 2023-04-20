import QtQuick
import QtQuick.Controls as QtQuick
import cdt
import "." as Cdt

QtQuick.Button {
  property string buttonIcon: ""
  padding: Theme.basePadding
  background: Rectangle {
    color: enabled && hovered && !pressed ? Theme.colorBgLight : "transparent"
    border.color: parent.activeFocus ? Theme.colorHighlight : "transparent"
    border.width: parent.activeFocus ? 2 : 0
    radius: Theme.baseRadius
  }
  contentItem: Cdt.Icon {
    icon: buttonIcon
    color: !parent.enabled ? Theme.colorSubText : (parent.checked ? Theme.colorHighlight : Theme.colorText)
  }
  Keys.onReturnPressed: checkable ? toggle() : clicked()
  Keys.onEnterPressed: checkable ? toggle() : clicked()
}
