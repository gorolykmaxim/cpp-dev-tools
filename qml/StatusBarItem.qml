import QtQuick
import "." as Cdt
import cdt

Row {
  id: root
  property string iconName: ""
  property string iconColor: Theme.colorText
  property bool displayIcon: false
  property string text: ""
  property string textColor: Theme.colorText
  spacing: Theme.basePadding
  Cdt.Icon {
    visible: root.displayIcon
    icon: root.iconName
    color: root.iconColor
  }
  Cdt.Text {
    text: root.text
    color: root.textColor
  }
}
