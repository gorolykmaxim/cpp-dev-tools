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
      TextFieldWidget {
        id: input
        text: vFilter || ""
        placeholderText: "Search project"
        focus: true
        Layout.fillWidth: true
        KeyNavigation.right: button
        onDisplayTextChanged: core.OnAction("vaFilterChanged", [displayText])
        Keys.onReturnPressed: Cmn.onListAction(list, "vaProjectSelected", "idx")
        Keys.onEnterPressed: Cmn.onListAction(list, "vaProjectSelected", "idx")
        Keys.onDownPressed: list.incrementCurrentIndex()
        Keys.onUpPressed: list.decrementCurrentIndex()
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
      id: list
      anchors.fill: parent
      model: vProjects
      onItemClicked: Cmn.callListActionOrOpenContextMenu(event, list,
                                                         "vaProjectSelected",
                                                         "idx", contextMenu)
    }
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Open"
      shortcut: "Enter"
      onTriggered: Cmn.onListAction(list, "vaProjectSelected", "idx")
    }
    MenuItem {
      text: "Remove From List"
      shortcut: "Ctrl+Shift+D"
      onTriggered: Cmn.onListAction(list, "vaRemoveProject", "idx")
    }
  }
}
