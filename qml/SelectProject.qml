import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.platform
import cdt
import "." as Cdt

ColumnLayout {
  Component.onCompleted: {
    appWindow.title = "Open Project";
    input.forceActiveFocus();
  }
  anchors.fill: parent
  spacing: 0
  ProjectsController {
    id: controller
  }
  Cdt.Pane {
    Layout.fillWidth: true
    color: Theme.colorBgMedium
    padding: Theme.basePadding
    RowLayout {
      anchors.fill: parent
      Cdt.ListSearch {
        id: input
        text: ""
        placeholderText: "Search project"
        Layout.fillWidth: true
        KeyNavigation.right: button
        list: projectList
        onDisplayTextChanged: console.log(displayText)
        onEnterPressed: console.log("ENTER")
        onCtrlEnterPressed: console.log("CTRL ENTER")
      }
      Cdt.Button {
        id: button
        text: "New Project"
        onClicked: console.log(text)
      }
    }
  }
  Cdt.Pane {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Theme.colorBgDark
    Cdt.TextList {
      id: projectList
      anchors.fill: parent
      model: controller.projects
      onItemLeftClicked: console.log("LEFT CLICK")
      onItemRightClicked: contextMenu.open()
    }
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Open"
      shortcut: "Enter"
      onTriggered: console.log(text)
    }
    MenuItem {
      text: "Remove From List"
      shortcut: "Ctrl+Shift+D"
      onTriggered: console.log(text)
    }
  }
}
