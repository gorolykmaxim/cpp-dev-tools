import QtQuick
import QtQuick.Controls
import cdt
import "." as Cdt

ApplicationWindow {
  id: appWindow
  property var currentProject: null
  minimumWidth: 1024
  minimumHeight: 600
  title: "Test"
  visible: true
  FontLoader {
    id: iconFont
    source: "../fonts/MaterialIcons-Regular.ttf"
  }
  Page {
    anchors.fill: parent
    background: Rectangle {
      color: Theme.colorBgBlack
    }
    Loader {
      id: currentView
      anchors.fill: parent
      source: "SelectProject.qml"
    }
    footer: Cdt.StatusBar {
      project: currentProject
    }
  }
}
