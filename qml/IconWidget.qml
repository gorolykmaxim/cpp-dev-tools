import QtQuick
import "MaterialDesignIcons.js" as MD

Text {
  property string icon: ""
  font.family: iconFont.name
  font.pixelSize: 48
  text: MD.icons[icon]
}
