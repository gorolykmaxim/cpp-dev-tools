import QtQuick
import Qt.labs.platform
import "." as Cdt
import cdt

Cdt.SearchableTextList {
  id: list
  anchors.fill: parent
  searchPlaceholderText: "Search task"
  searchableModel: controller.tasks
  focus: true
  onItemRightClicked: contextMenu.open()
  onItemSelected: taskSystem.executeTask(controller.selectedTaskIndex, false)
  TaskListController {
    id: controller
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Run"
      onTriggered: taskSystem.executeTask(controller.selectedTaskIndex, false)
    }
    MenuItem {
      text: "Run Until Fails"
      shortcut: "Alt+Shift+R"
      onTriggered: taskSystem.executeTask(controller.selectedTaskIndex, true)
    }
  }
}
