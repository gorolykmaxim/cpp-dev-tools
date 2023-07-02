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
          shortcut: "Alt+Y"
          onTriggered: listModel.executeCurrentTask(false, "QtestExecution.qml", [])
        }
        MenuItem {
          text: "Run as QtTest With Filter"
          shortcut: "Ctrl+Alt+Y"
          onTriggered: root.sourceComponent = qtestFilterView
        }
        MenuItem {
          text: "Run Until Fails"
          shortcut: "Alt+Shift+R"
          onTriggered: listModel.executeCurrentTask(true, "TaskExecution.qml", [])
        }
        MenuItem {
          text: "Run as QtTest Until Fails"
          shortcut: "Alt+Shift+Y"
          onTriggered: listModel.executeCurrentTask(true, "QtestExecution.qml", [])
        }
        MenuItem {
          text: "Run as QtTest With Filter Until Fails"
          shortcut: "Ctrl+Alt+Shift+Y"
          onTriggered: root.sourceComponent = qtestFilterUntilFailView
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
}
