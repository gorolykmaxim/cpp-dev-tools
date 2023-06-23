import QtQuick
import QtQuick.Controls as QtQuick

QtQuick.Pane {
  property string color: "transparent"
  property alias border: bg.border
  padding: 0
  background: Rectangle {
    id: bg
    color: parent.color
  }
}
