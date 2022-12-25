import QtQuick
import QtQuick.Controls

ApplicationWindow {
  property var basePadding: 4
  property var baseRadius: 4
  property var colorBgLight: "#585859"
  property var colorBgMedium: "#383838"
  property var colorBgDark: "#282828"
  property var colorBgBlack: "#191919"
  property var colorText: "#efefef"
  property var colorSubText: "#6d6d6d"
  property var colorHighlight: "#9ab8ef"
  minimumWidth: 1024
  minimumHeight: 600
  title: windowTitle
  visible: true
  FontLoader {
    id: iconFont
    source: "../fonts/MaterialIcons-Regular.ttf"
  }
  menuBar: MenuBarWidget {}
  Page {
    anchors.fill: parent
    background: Rectangle {
      color: colorBgBlack
    }
    Loader {
      id: viewLoader
      anchors.fill: parent
      source: viewSlot
      onLoaded: {
        if (!dialog.visible) {
          forceActiveFocus();
        }
      }
    }
    Dialog {
      id: dialog
      width: 500
      height: dFixedHeight ?
              parent.height * 0.8 :
              Math.min(contentHeight + padding * 2, parent.height * 0.8)
      padding: dPadding ? basePadding * 2 : 0
      modal: true
      visible: dVisible ?? false
      anchors.centerIn: parent
      background: Rectangle {
        color: colorBgMedium
        radius: baseRadius
        border.width: 1
        border.color: colorBgLight
      }
      onVisibleChanged: {
        if (visible) {
          dialogLoader.forceActiveFocus();
        } else {
          viewLoader.forceActiveFocus();
        }
      }
      onAccepted: core.OnAction("daOk", [])
      onRejected: core.OnAction("daCancel", [])
      Loader {
        id: dialogLoader
        anchors.fill: parent
        anchors.margins: 1
        source: dialogSlot
        onLoaded: forceActiveFocus()
      }
    }
    footer: StatusBarWidget {}
  }
}
