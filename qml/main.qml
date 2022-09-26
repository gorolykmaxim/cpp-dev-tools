import QtQuick
import QtQuick.Controls

Window {
  property var basePadding: 4
  property var baseRadius: 4
  property var colorBgLight: "#585859"
  property var colorBgMedium: "#383838"
  property var colorBgDark: "#282828"
  property var colorBgBlack: "#191919"
  property var colorText: "#efefef"
  property var colorHighlight: "#9ab8ef"
  width: 1024
  height: 600
  title: "CPP Dev Tools"
  visible: true
  FontLoader {
    id: iconFont
    source: "../fonts/MaterialIcons-Regular.ttf"
  }
  Page {
    anchors.fill: parent
    background: Rectangle {
      color: colorBgBlack
    }
    Loader {
      id: loader
      anchors.fill: parent
      source: currentView
      onLoaded: forceActiveFocus()
    }
    DialogWidget {
      onVisibleChanged: {
        if (visible) {
          forceActiveFocus();
        } else {
          loader.forceActiveFocus();
        }
      }
    }
    footer: TextWidget {
      padding: basePadding
      text: "I'm in footer"
    }
  }
}
