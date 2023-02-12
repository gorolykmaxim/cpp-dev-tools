import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.platform
import "Common.js" as Cmn

ColumnLayout {
  anchors.fill: parent
  spacing: 0
  PaneWidget {
    Layout.fillWidth: true
    color: colorBgMedium
    focus: true
    padding: basePadding
    RowLayout {
      anchors.fill: parent
      ListSearchWidget {
        id: input
        text: vFilter || ""
        placeholderText: "Search project"
        focus: true
        Layout.fillWidth: true
        KeyNavigation.right: button
        list: projectList
        changeEventType: "vaFilterChanged"
        enterEventType: "vaProjectSelected"
        listIdFieldName: "idx"
      }
      ButtonWidget {
        id: button
        text: "New Project"
        onClicked: core.OnAction("vaNewProject", [])
      }
    }
  }
  PaneWidget {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: colorBgDark
    TextListWidget {
      id: projectList
      anchors.fill: parent
      model: vProjects
      onItemLeftClicked: Cmn.onListAction(projectList, "vaProjectSelected",
                                          "idx")
      onItemRightClicked: contextMenu.open()
    }
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Open"
      shortcut: "Enter"
      onTriggered: Cmn.onListAction(projectList, "vaProjectSelected", "idx")
    }
    MenuItem {
      text: "Remove From List"
      shortcut: "Ctrl+Shift+D"
      onTriggered: Cmn.onListAction(projectList, "vaRemoveProject", "idx")
    }
  }
}
