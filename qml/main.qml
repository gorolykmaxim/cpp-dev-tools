import QtQuick
import QtQuick.Controls
import cdt
import "." as Cdt

ApplicationWindow {
  title: viewController.windowTitle
  minimumWidth: 1024
  minimumHeight: 600
  visible: true
  FontLoader {
    id: iconFont
    source: "../fonts/MaterialIcons-Regular.ttf"
  }
  menuBar: Cdt.MenuBar {
    isProjectOpened: projectContext.isProjectOpened
    model: userCommands.userCommands
  }
  Page {
    anchors.fill: parent
    background: Rectangle {
      color: Theme.colorBgBlack
    }
    Loader {
      anchors.fill: parent
      source: viewController.currentView
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
