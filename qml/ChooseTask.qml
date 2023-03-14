import QtQuick
import "." as Cdt
import cdt

Cdt.SearchableTextList {
  Component.onCompleted: forceActiveFocus()
  signal taskSelected(string command)
  anchors.fill: parent
  searchPlaceholderText: "Search task"
  showPlaceholder: controller.showPlaceholder
  placeholderText: controller.isLoading ? "Looking for tasks..." : "No tasks found"
  searchableModel: controller.tasks
  onItemSelected: ifCurrentItem('command', taskSelected)
  ChooseTaskController {
    id: controller
  }
}
