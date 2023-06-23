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
    gradient: Gradient {
      GradientStop { position: 0.0; color: root.checked ? Theme.colorButtonPrimaryLight : "#4f4f50" }
      GradientStop { position: 1.0; color: root.checked ? Theme.colorButtonPrimaryDark : "#6b6b6c" }
    }
    border.color: root.checked || root.activeFocus ? Theme.colorButtonPrimaryDark : Theme.colorButtonBorder
    border.width: 1
    radius: Theme.baseRadius
    Cdt.Icon {
      icon: "check"
      iconSize: root.size
      visible: root.checked
      color: !root.enabled ? Theme.colorPlaceholder : Theme.colorText
    }
  }
  contentItem: Cdt.Text {
    text: root.text
    color: !root.enabled ? Theme.colorPlaceholder : (root.activeFocus ? Theme.colorPrimary : Theme.colorText)
    leftPadding: root.indicator.width + Theme.basePadding
    verticalAlignment: Text.AlignVCenter
  }
  Keys.onEnterPressed: toggle()
  Keys.onReturnPressed: toggle()
}
