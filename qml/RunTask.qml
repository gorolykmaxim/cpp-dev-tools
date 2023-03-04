import QtQuick
import "." as Cdt
import cdt

Cdt.SearchableTextList {
  Component.onCompleted: {
    appWindow.title = "Run Task";
    forceActiveFocus();
    controller.FindTasks()
  }
  anchors.fill: parent
  searchPlaceholderText: "Search task"
  showPlaceholder: controller.showPlaceholder
  placeholderText: controller.isLoading ? "Looking for tasks..." : "No tasks found"
  searchableModel: controller.tasks
  onItemSelected: ifCurrentItem('idx', controller.ExecTask)
  ChooseTaskController {
    id: controller
  }
}
