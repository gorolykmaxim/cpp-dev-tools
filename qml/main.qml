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
  Cdt.UserCommands {
    id: userCommands
  }
  menuBar: Cdt.MenuBar {
    project: currentProject
    model: userCommands.userCommands
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
      onSourceChanged: searchUserCommandDialog.reject()
    }
    footer: Cdt.StatusBar {
      project: currentProject
    }
  }
  Cdt.SearchUserCommandDialog {
    id: searchUserCommandDialog
    userCommands: userCommands.userCommands
  }
}
