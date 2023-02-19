import QtQuick
import QtQuick.Controls
import cdt

ApplicationWindow {
  minimumWidth: 1024
  minimumHeight: 600
  title: "Test"
  visible: true
  InformationProvider {
    id: provider
  }
  Text {
    anchors.centerIn: parent
    text: provider.info
  }
}
