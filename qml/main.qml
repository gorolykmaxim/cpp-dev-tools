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
  property var colorSubText: "#6d6d6d"
  property var colorHighlight: "#9ab8ef"
  width: 1024
  height: 600
  title: windowTitle
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
      id: viewLoader
      anchors.fill: parent
      source: viewSlot
      onLoaded: forceActiveFocus()
    }
    Dialog {
      id: dialog
      width: parent.width / 2
      modal: true
      visible: dataDialogVisible ?? false
      anchors.centerIn: parent
      background: Rectangle {
        color: colorBgMedium
        radius: baseRadius
      }
      onVisibleChanged: {
        if (visible) {
          dialogLoader.forceActiveFocus();
        } else {
          viewLoader.forceActiveFocus();
        }
      }
      Loader {
        id: dialogLoader
        anchors.fill: parent
        source: dialogSlot
        onLoaded: forceActiveFocus()
      }
    }
    footer: StatusBarWidget {}
  }
}
