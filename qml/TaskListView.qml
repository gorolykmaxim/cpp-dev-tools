import QtQuick
import QtQuick.Layouts
import Qt.labs.platform
import "Common.js" as Cmn

ColumnLayout {
  anchors.fill: parent
  spacing: 0
  TextWidget {
    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
    visible: vLoading
    text: "Looking for tasks..."
  }
  PaneWidget {
    Layout.fillWidth: true
    color: colorBgMedium
    focus: true
    padding: basePadding
    visible: !vLoading
    TextFieldWidget {
        id: input
        text: vFilter || ""
        placeholderText: "Search task"
        focus: true
        anchors.fill: parent
        onDisplayTextChanged: core.OnAction("vaFilterChanged", [displayText])
        Keys.onReturnPressed: e => {
          if (e.modifiers & Qt.ControlModifier) {
            Cmn.onListAction(list, "vaExecuteTaskAndDisplay", "idx");
          } else {
            Cmn.onListAction(list, "vaExecuteTask", "idx");
          }
        }
        Keys.onEnterPressed: Cmn.onListAction(list, "vaExecuteTask", "idx")
        Keys.onDownPressed: list.incrementCurrentIndex()
        Keys.onUpPressed: list.decrementCurrentIndex()
      }
  }
  PaneWidget {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: colorBgDark
    visible: !vLoading
    TextListWidget {
      id: list
      anchors.fill: parent
      model: vTasks
      onItemClicked: Cmn.callListActionOrOpenContextMenu(event, list,
                                                         "vaExecuteTask",
                                                         "idx", contextMenu)
    }
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Execute"
      shortcut: "Enter"
      onTriggered: Cmn.onListAction(list, "vaExecuteTask", "idx")
    }
    MenuItem {
      text: "Execute"
      shortcut: "Ctrl+Enter"
      onTriggered: Cmn.onListAction(list, "vaExecuteTaskAndDisplay", "idx")
    }
  }
}
