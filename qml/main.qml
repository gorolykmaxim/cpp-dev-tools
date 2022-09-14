import QtQuick
import QtQuick.Controls

Window {
  property var globalPadding: 4
  width: 1024
  height: 600
  title: "CPP Dev Tools"
  visible: true
  Page {
    anchors.fill: parent
    Loader {
      anchors.fill: parent
      source: currentView
      onLoaded: forceActiveFocus()
    }
    footer: Label {
      padding: globalPadding
      text: "I'm in footer"
    }
  }
}
