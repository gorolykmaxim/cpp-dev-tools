import QtQuick
import QtQuick.Controls

Window {
  property var globalPadding: 4
  width: 1024
  height: 600
  title: "CPP Dev Tools"
  visible: true
  SystemPalette {
    id: palette;
    colorGroup: SystemPalette.Active
  }
  FontLoader {
    id: iconFont
    source: "../fonts/MaterialIcons-Regular.ttf"
  }
  Page {
    anchors.fill: parent
    Loader {
      id: loader
      anchors.fill: parent
      source: currentView
      onLoaded: forceActiveFocus()
    }
    DialogMac {
      onVisibleChanged: {
        if (visible) {
          forceActiveFocus();
        } else {
          loader.forceActiveFocus();
        }
      }
    }
    footer: Label {
      padding: globalPadding
      text: "I'm in footer"
    }
  }
}
