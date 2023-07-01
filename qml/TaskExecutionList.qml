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
  searchableModel: listModel
  focus: true
  onItemSelected: listModel.openExecutionOutput()
  onItemRightClicked: Common.handleRightClick(execList, contextMenu)
  TaskExecutionListModel {
    id: listModel
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Open Output"
      enabled: execList.activeFocus
      shortcut: "Enter"
      onTriggered: listModel.openExecutionOutput()
    }
    MenuItem {
      text: "Re-Run"
      enabled: execList.activeFocus
      shortcut: "Alt+Shift+R"
      onTriggered: listModel.rerunSelectedExecution(false)
    }
    MenuItem {
      text: "Re-Run Until Fails"
      enabled: execList.activeFocus
      shortcut: "Ctrl+Alt+Shift+R"
      onTriggered: listModel.rerunSelectedExecution(true)
    }
    MenuItem {
      text: "Terminate"
      enabled: execList.activeFocus && listModel.executionRunning
      shortcut: "Alt+Shift+T"
      onTriggered: taskSystem.cancelSelectedExecution(id, false)
    }
    MenuItem {
      text: "Kill"
      enabled: execList.activeFocus && listModel.executionRunning
      shortcut: "Alt+Shift+K"
      onTriggered: taskSystem.cancelSelectedExecution(id, true)
    }
    MenuItem {
      text: "Remove Finished Executions"
      enabled: execList.activeFocus
      shortcut: "Alt+Shift+D"
      onTriggered: listModel.removeFinishedExecutions()
    }
  }
}
