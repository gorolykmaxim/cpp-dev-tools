import QtQuick
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt
import "Common.js" as Common

Cdt.SearchableTextList {
  id: execList
  anchors.fill: parent
  searchPlaceholderText: "Search execution"
  placeholderText: "No tasks have been executed yet"
  showPlaceholder: controller.executionsEmpty
  searchableModel: controller.executions
  focus: true
  onItemSelected: controller.openExecutionOutput()
  onItemRightClicked: Common.handleRightClick(execList, contextMenu)
  TaskExecutionListController {
    id: controller
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Open Output"
      enabled: execList.activeFocus
      shortcut: "Enter"
      onTriggered: controller.openExecutionOutput()
    }
    MenuItem {
      text: "Re-Run"
      enabled: execList.activeFocus && controller.executionTaskIndex >= 0
      shortcut: "Alt+Shift+R"
      onTriggered: taskSystem.executeTask(controller.executionTaskIndex)
    }
    MenuItem {
      text: "Re-Run Until Fails"
      enabled: execList.activeFocus && controller.executionTaskIndex >= 0
      shortcut: "Ctrl+Alt+Shift+R"
      onTriggered: taskSystem.executeTask(controller.executionTaskIndex, true)
    }
    MenuItem {
      text: "Terminate"
      enabled: execList.activeFocus && controller.executionRunning
      shortcut: "Alt+Shift+T"
      onTriggered: taskSystem.cancelSelectedExecution(id, false)
    }
    MenuItem {
      text: "Kill"
      enabled: execList.activeFocus && controller.executionRunning
      shortcut: "Alt+Shift+K"
      onTriggered: taskSystem.cancelSelectedExecution(id, true)
    }
    MenuItem {
      text: "Remove Finished Executions"
      enabled: execList.activeFocus
      shortcut: "Alt+Shift+D"
      onTriggered: controller.removeFinishedExecutions()
    }
  }
}
