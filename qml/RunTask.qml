import QtQuick
import "." as Cdt
import cdt

Loader {
  id: root
  Component.onCompleted: {
    appWindow.title = "Run Task";
    controller.FindTasks()
  }
  anchors.fill: parent
  sourceComponent: controller.showPlaceholder ? placeholderComponent : taskListComponent
  ChooseTaskController {
    id: controller
    project: appWindow.currentProject
  }
  Component {
    id: placeholderComponent
    Cdt.Text {
      anchors.centerIn: parent
      text: controller.isLoading ? "Looking for tasks..." : "No tasks found"
    }
  }
  Component {
    id: taskListComponent
    Cdt.SearchableTextList {
      anchors.fill: parent
      placeholderText: "Search task"
      searchableModel: controller.tasks
      onItemSelected: console.log("EXECUTING TASK")
      Component.onCompleted: forceActiveFocus()
    }
  }
}
