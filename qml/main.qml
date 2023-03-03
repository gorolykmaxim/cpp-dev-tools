import QtQuick
import QtQuick.Controls
import cdt
import "." as Cdt

ApplicationWindow {
  id: appWindow
  minimumWidth: 1024
  minimumHeight: 600
  title: "Test"
  visible: true
  FontLoader {
    id: iconFont
    source: "../fonts/MaterialIcons-Regular.ttf"
  }
  Cdt.UserCommands {
    id: userCommands
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
      onSourceChanged: searchUserCommandDialog.reject()
    }
    footer: Cdt.StatusBar {}
  }
  Cdt.SearchUserCommandDialog {
    id: searchUserCommandDialog
    userCommands: userCommands.userCommands
  }
}
