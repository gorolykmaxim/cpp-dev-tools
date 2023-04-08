import QtQuick
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt
import "Common.js" as Common

RowLayout {
  anchors.fill: parent
  spacing: 0
  TaskExecutionListController {
    id: controller
  }
  Cdt.Text {
    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
    visible: controller.executionsEmpty
    text: "No tasks have been executed yet"
  }
  Cdt.SearchableTextList {
    id: execList
    visible: !controller.executionsEmpty
    Layout.fillWidth: true
    Layout.fillHeight: true
    searchPlaceholderText: "Search execution"
    searchableModel: controller.executions
    focus: true
    onCurrentItemChanged: ifCurrentItem('id', (id) => taskSystem.selectedExecutionId = id)
    onItemSelected: controller.OpenExecutionOutput()
    onItemRightClicked: Common.handleRightClick(execList, contextMenu)
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Open Output"
      enabled: execList.activeFocus
      shortcut: "Enter"
      onTriggered: controller.OpenExecutionOutput()
    }
    MenuItem {
      text: "Re-Run"
      enabled: execList.activeFocus && controller.executionTaskIndex >= 0
      shortcut: "Alt+Shift+R"
      onTriggered: taskSystem.ExecuteTask(controller.executionTaskIndex)
    }
    MenuItem {
      text: "Re-Run Until Fails"
      enabled: execList.activeFocus && controller.executionTaskIndex >= 0
      shortcut: "Ctrl+Alt+Shift+R"
      onTriggered: taskSystem.ExecuteTask(controller.executionTaskIndex, true)
    }
    MenuItem {
      text: "Terminate"
      enabled: execList.activeFocus && controller.executionRunning
      shortcut: "Alt+Shift+T"
      onTriggered: execList.ifCurrentItem('id', (id) => taskSystem.CancelExecution(id, false))
    }
    MenuItem {
      text: "Kill"
      enabled: execList.activeFocus && controller.executionRunning
      shortcut: "Alt+Shift+K"
      onTriggered: execList.ifCurrentItem('id', (id) => taskSystem.CancelExecution(id, true))
    }
    MenuItem {
      text: "Remove Finished Executions"
      enabled: execList.activeFocus
      shortcut: "Alt+Shift+D"
      onTriggered: controller.RemoveFinishedExecutions()
    }
  }
}
