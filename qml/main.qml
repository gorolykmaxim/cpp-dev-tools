import QtQuick
import QtQuick.Controls
import cdt
import "." as Cdt

ApplicationWindow {
  id: appWindow
  Component.onCompleted: applyShowMode()
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
  onActiveChanged: {
    if (active) {
      // The window has regained focus
      gitSystem.refreshBranchesIfProjectSelected();
    }
  }
  Connections {
    target: viewSystem
    function onWindowDimensionsChanaged() {
      applyShowMode();
    }
  }
  function applyShowMode() {
    if (viewSystem.dimensions.isMaximized) {
      showMaximized();
    } else {
      showNormal();
    }
  }
  Timer {
    id: windowDimensionsDebounce
    interval: 100
    onTriggered: viewSystem.saveWindowDimensions(width, height, x, y,
                                                 visibility == Window.Maximized)
  }
  menuBar: Cdt.MenuBar {
    isProjectOpened: projectSystem.isProjectOpened
    model: userCommandSystem.userCommands
  }
  Page {
    anchors.fill: parent
    focus: !searchUserCommandDialog.visible && !alertDialog.visible
    background: Rectangle {
      color: Theme.colorBackground
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
