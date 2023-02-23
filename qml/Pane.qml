import QtQuick
import QtQuick.Controls

Pane {
  property var color: "transparent"
  padding: 0
  background: Rectangle {
    color: parent.color
  }
}
