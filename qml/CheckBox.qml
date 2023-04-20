import QtQuick
import QtQuick.Controls as QtQuick
import "." as Cdt
import cdt

QtQuick.CheckBox {
  property int size: 14
  id: root
  padding: 0
  indicator: Rectangle {
    implicitWidth: root.size
    implicitHeight: root.size
    x: root.leftPadding
    y: root.height / 2 - height / 2
    color: Theme.colorBgDark
    border.color: root.activeFocus ? Theme.colorHighlight : Theme.colorBgBlack
    border.width: root.activeFocus ? 2 : 1
    radius: Theme.baseRadius
    Cdt.Icon {
      icon: "check"
      iconSize: root.size
      visible: root.checked
      color: !root.enabled ? Theme.colorSubText : (root.activeFocus ? Theme.colorHighlight : Theme.colorText)
    }
  }
  contentItem: Cdt.Text {
    text: root.text
    color: !root.enabled ? Theme.colorSubText : (root.activeFocus ? Theme.colorHighlight : Theme.colorText)
    leftPadding: root.indicator.width + Theme.basePadding
    verticalAlignment: Text.AlignVCenter
  }
  Keys.onEnterPressed: toggle()
  Keys.onReturnPressed: toggle()
}
