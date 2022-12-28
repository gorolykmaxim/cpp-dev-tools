import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.platform

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
        Keys.onReturnPressed: core.OnAction("vaProjectSelected",
                                            [list.currentItem.itemModel.idx])
        Keys.onEnterPressed: core.OnAction("vaProjectSelected",
                                           [list.currentItem.itemModel.idx])
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
      onItemClicked: (item, event) => {
        if (event.button == Qt.LeftButton) {
          core.OnAction("vaProjectSelected", [item.idx])
        } else if (event.button == Qt.RightButton) {
          contextMenu.open();
        }
      }
    }
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Open"
      shortcut: "Enter"
      onTriggered: core.OnAction("vaProjectSelected",
                                 [list.currentItem.itemModel.idx])
    }
    MenuItem {
      text: "Remove From List"
      shortcut: "Ctrl+Shift+D"
      onTriggered: core.OnAction("vaRemoveProject",
                                 [list.currentItem.itemModel.idx])
    }
  }
}
