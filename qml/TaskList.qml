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
  onItemSelected: listModel.executeCurrentTask(false)
  TaskListModel {
    id: listModel
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Run"
      onTriggered: listModel.executeCurrentTask(false)
    }
    MenuItem {
      text: "Run Until Fails"
      shortcut: "Alt+Shift+R"
      onTriggered: listModel.executeCurrentTask(true)
    }
  }
}
