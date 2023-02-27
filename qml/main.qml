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
  ProjectContext {
    id: projectContext
  }
  menuBar: Cdt.MenuBar {
    project: projectContext.currentProject
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
      function display(url) {
        source = "";
        source = url;
      }
      onSourceChanged: searchUserCommandDialog.reject()
    }
    footer: Cdt.StatusBar {
      project: projectContext.currentProject
    }
  }
  Cdt.SearchUserCommandDialog {
    id: searchUserCommandDialog
    userCommands: userCommands.userCommands
  }
}
