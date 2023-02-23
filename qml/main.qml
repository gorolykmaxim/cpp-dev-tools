import QtQuick
import QtQuick.Controls
import cdt
import "." as Cdt

ApplicationWindow {
  minimumWidth: 1024
  minimumHeight: 600
  title: "Test"
  visible: true
  InformationProvider {
    id: provider
  }
  Cdt.Component {
    anchors.centerIn: parent
    text: provider.info
    color: Theme.colorSubText
  }
}
