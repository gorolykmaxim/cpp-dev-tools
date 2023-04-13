import QtQuick
import QtQuick.Controls
import cdt
import "." as Cdt

ApplicationWindow {
  Component.onCompleted: {
    if (viewSystem.dimensions.isMaximized) {
      showMaximized();
    }
  }
  title: viewSystem.windowTitle
  width: viewSystem.dimensions.width
  height: viewSystem.dimensions.height
  x: viewSystem.dimensions.x
  y: viewSystem.dimensions.y
  onWidthChanged: windowDimensionsDebounce.restart()
  onHeightChanged: windowDimensionsDebounce.restart()
  onXChanged: windowDimensionsDebounce.restart()
  onYChanged: windowDimensionsDebounce.restart()
  visible: true
  Timer {
    id: windowDimensionsDebounce
    interval: 100
    onTriggered: viewSystem.saveWindowDimensions(width, height, x, y,
                                                 visibility == Window.Maximized)
  }
  FontLoader {
    id: iconFont
    source: "../fonts/MaterialIcons-Regular.ttf"
  }
  menuBar: Cdt.MenuBar {
    isProjectOpened: projectSystem.isProjectOpened
    model: userCommandSystem.userCommands
  }
  Page {
    anchors.fill: parent
    focus: !searchUserCommandDialog.visible && !alertDialog.visible
    background: Rectangle {
      color: Theme.colorBgBlack
    }
    Loader {
      anchors.fill: parent
      source: viewSystem.currentView
      focus: true
    }
    footer: Cdt.StatusBar {}
  }
  Cdt.SearchUserCommandDialog {
    id: searchUserCommandDialog
  }
  Cdt.AlertDialog {
    id: alertDialog
  }
}
