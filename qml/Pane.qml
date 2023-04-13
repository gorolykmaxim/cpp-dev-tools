import QtQuick
import QtQuick.Controls as QtQuick

QtQuick.Pane {
  property string color: "transparent"
  padding: 0
  background: Rectangle {
    color: parent.color
  }
}
