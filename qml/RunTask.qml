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
    project: projectContext.currentProject
  }
  Component {
    id: placeholderComponent
    Cdt.Text {
      anchors.fill: parent
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
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
