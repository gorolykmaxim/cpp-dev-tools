import QtQuick
import Qt.labs.platform
import "." as Cdt
import cdt

Cdt.SearchableTextList {
  id: list
  anchors.fill: parent
  searchPlaceholderText: "Search task"
  showPlaceholder: controller.showPlaceholder
  placeholderText: controller.isLoading ? "Looking for tasks..." : "No tasks found"
  searchableModel: controller.tasks
  focus: true
  onItemRightClicked: contextMenu.open()
  onItemSelected: item => taskSystem.ExecuteTask(item.idx, false)
  TaskListController {
    id: controller
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Run"
      onTriggered: list.ifCurrentItem('idx', id => taskSystem.ExecuteTask(id, false))
    }
    MenuItem {
      text: "Run Until Fails"
      shortcut: "Alt+Shift+R"
      onTriggered: list.ifCurrentItem('idx', id => taskSystem.ExecuteTask(id, true))
    }
  }
}
