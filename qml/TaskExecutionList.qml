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
  onItemSelected: viewSystem.currentView = "TaskExecution.qml"
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
      onTriggered: viewSystem.currentView = "TaskExecution.qml"
    }
    MenuItem {
      text: "Open as QtTest"
      enabled: execList.activeFocus
      shortcut: gSC("TaskExecutionList", "Open as QtTest")
      onTriggered: viewSystem.currentView = "QtestExecution.qml"
    }
    MenuItem {
      text: "Open as Google Test"
      enabled: execList.activeFocus
      shortcut: gSC("TaskExecutionList", "Open as Google Test")
      onTriggered: viewSystem.currentView = "GtestExecution.qml"
    }
    MenuItem {
      text: "Re-Run"
      enabled: execList.activeFocus
      shortcut: gSC("TaskExecutionList", "Re-Run")
      onTriggered: listModel.rerunSelectedExecution(false, "TaskExecution.qml")
    }
    MenuItem {
      text: "Re-Run as QtTest"
      shortcut: gSC("TaskExecutionList", "Re-Run as QtTest")
      onTriggered: listModel.rerunSelectedExecution(false, "QtestExecution.qml")
    }
    MenuItem {
      text: "Re-Run as Google Test"
      shortcut: gSC("TaskExecutionList", "Re-Run as Google Test")
      onTriggered: listModel.rerunSelectedExecution(false, "GtestExecution.qml")
    }
    MenuItem {
      text: "Re-Run Until Fails"
      enabled: execList.activeFocus
      shortcut: gSC("TaskExecutionList", "Re-Run Until Fails")
      onTriggered: listModel.rerunSelectedExecution(true, "TaskExecution.qml")
    }
    MenuItem {
      text: "Re-Run as QtTest Until Fails"
      enabled: execList.activeFocus
      shortcut: gSC("TaskExecutionList", "Re-Run as QtTest Until Fails")
      onTriggered: listModel.rerunSelectedExecution(true, "QtestExecution.qml")
    }
    MenuItem {
      text: "Re-Run as Google Test Until Fails"
      enabled: execList.activeFocus
      shortcut: gSC("TaskExecutionList", "Re-Run as Google Test Until Fails")
      onTriggered: listModel.rerunSelectedExecution(true, "GtestExecution.qml")
    }
    MenuItem {
      text: "Terminate"
      enabled: execList.activeFocus && listModel.executionRunning
      shortcut: gSC("TaskExecutionList", "Terminate")
      onTriggered: taskSystem.cancelSelectedExecution(id, false)
    }
    MenuItem {
      text: "Kill"
      enabled: execList.activeFocus && listModel.executionRunning
      shortcut: gSC("TaskExecutionList", "Kill")
      onTriggered: taskSystem.cancelSelectedExecution(id, true)
    }
    MenuItem {
      text: "Remove Finished Executions"
      enabled: execList.activeFocus
      shortcut: gSC("TaskExecutionList", "Remove Finished Executions")
      onTriggered: listModel.removeFinishedExecutions()
    }
  }
}
