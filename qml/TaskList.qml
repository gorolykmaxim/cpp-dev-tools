import QtQuick
import Qt.labs.platform
import "." as Cdt
import cdt

Cdt.SearchableTextList {
  id: list
  anchors.fill: parent
  searchPlaceholderText: "Search task"
  searchableModel: listModel
  focus: true
  onItemRightClicked: contextMenu.open()
  onItemSelected: listModel.executeCurrentTask(false, "TaskExecution.qml")
  TaskListModel {
    id: listModel
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Run"
      onTriggered: listModel.executeCurrentTask(false, "TaskExecution.qml")
    }
    MenuItem {
      text: "Run as QtTest"
      shortcut: "Alt+Y"
      onTriggered: listModel.executeCurrentTask(false, "QtestExecution.qml")
    }
    MenuItem {
      text: "Run Until Fails"
      shortcut: "Alt+Shift+R"
      onTriggered: listModel.executeCurrentTask(true, "TaskExecution.qml")
    }
    MenuItem {
      text: "Run as QtTest Until Fails"
      shortcut: "Alt+Shift+Y"
      onTriggered: listModel.executeCurrentTask(true, "QtestExecution.qml")
    }
  }
}
