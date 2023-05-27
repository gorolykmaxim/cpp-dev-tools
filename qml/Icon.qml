import QtQuick
import "MaterialDesignIcons.js" as MD

Text {
  property string icon: ""
  property real iconSize: 16
  font.family: "Material Icons"
  font.pixelSize: iconSize
  text: MD.icons[icon] || ""
  renderType: Text.NativeRendering
}
