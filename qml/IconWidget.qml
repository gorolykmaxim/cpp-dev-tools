import QtQuick
import "MaterialDesignIcons.js" as MD

Text {
  property string icon: ""
  property real iconSize: 20
  font.family: iconFont.name
  font.pixelSize: iconSize
  text: MD.icons[icon] || ""
}
