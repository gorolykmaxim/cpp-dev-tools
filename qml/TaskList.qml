import QtQuick
import Qt.labs.platform
import "." as Cdt
import cdt

Loader {
  id: root
  anchors.fill: parent
  focus: true
  sourceComponent: listView
  TaskListModel {
    id: listModel
  }
  Component {
    id: listView
    Cdt.SearchableTextList {
      anchors.fill: parent
      searchPlaceholderText: "Search task"
      searchableModel: listModel
      focus: true
      onItemRightClicked: contextMenu.open()
      onItemSelected: listModel.executeCurrentTask(false, "TaskExecution.qml", [])
      Component.onCompleted: listModel.displayTaskList()
      Menu {
        id: contextMenu
        MenuItem {
          text: "Run"
          onTriggered: listModel.executeCurrentTask(false, "TaskExecution.qml", [])
        }
        MenuItem {
          text: "Run as QtTest"
          shortcut: gSC("TaskList", "Run as QtTest")
          onTriggered: listModel.executeCurrentTask(false, "QtestExecution.qml", [])
        }
        MenuItem {
          text: "Run as QtTest With Filter"
          shortcut: gSC("TaskList", "Run as QtTest With Filter")
          onTriggered: root.sourceComponent = qtestFilterView
        }
        MenuItem {
          text: "Run as Google Test"
          shortcut: gSC("TaskList", "Run as Google Test")
          onTriggered: listModel.executeCurrentTask(false, "GtestExecution.qml", [])
        }
        MenuItem {
          text: "Run as Google Test With Filter"
          shortcut: gSC("TaskList", "Run as Google Test With Filter")
          onTriggered: root.sourceComponent = gtestFilterView
        }
        MenuItem {
          text: "Run Until Fails"
          shortcut: gSC("TaskList", "Run Until Fails")
          onTriggered: listModel.executeCurrentTask(true, "TaskExecution.qml", [])
        }
        MenuItem {
          text: "Run as QtTest Until Fails"
          shortcut: gSC("TaskList", "Run as QtTest Until Fails")
          onTriggered: listModel.executeCurrentTask(true, "QtestExecution.qml", [])
        }
        MenuItem {
          text: "Run as QtTest With Filter Until Fails"
          shortcut: gSC("TaskList", "Run as QtTest With Filter Until Fails")
          onTriggered: root.sourceComponent = qtestFilterUntilFailView
        }
        MenuItem {
          text: "Run as Google Test Until Fails"
          shortcut: gSC("TaskList", "Run as Google Test Until Fails")
          onTriggered: listModel.executeCurrentTask(true, "GtestExecution.qml", [])
        }
        MenuItem {
          text: "Run as Google Test With Filter Until Fails"
          shortcut: gSC("TaskList", "Run as Google Test With Filter Until Fails")
          onTriggered: root.sourceComponent = gtestFilterUntilFailView
        }
      }
    }
  }
  Component {
    id: qtestFilterView
    Cdt.SetTestFilter {
      windowTitle: "Run QtTest With Filter"
      onFilterChosen: filter => listModel.executeCurrentTask(false, "QtestExecution.qml", [filter])
      onBack: root.sourceComponent = listView
    }
  }
  Component {
    id: qtestFilterUntilFailView
    Cdt.SetTestFilter {
      windowTitle: "Run QtTest With Filter Until Fails"
      onFilterChosen: filter => listModel.executeCurrentTask(true, "QtestExecution.qml", [filter])
      onBack: root.sourceComponent = listView
    }
  }
  Component {
    id: gtestFilterView
    Cdt.SetTestFilter {
      windowTitle: "Run Google Test With Filter"
      onFilterChosen: filter => listModel.executeCurrentTask(false, "GtestExecution.qml", ["--gtest_filter=" + filter])
      onBack: root.sourceComponent = listView
    }
  }
  Component {
    id: gtestFilterUntilFailView
    Cdt.SetTestFilter {
      windowTitle: "Run Google Test With Filter Until Fails"
      onFilterChosen: filter => listModel.executeCurrentTask(true, "GtestExecution.qml", ["--gtest_filter=" + filter])
      onBack: root.sourceComponent = listView
    }
  }
}
