import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt
import "Common.js" as Common

RowLayout {
  anchors.fill: parent
  spacing: 0
  TaskExecutionHistoryController {
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
    Layout.maximumWidth: 300
    Layout.minimumWidth: 300
    Layout.fillHeight: true
    searchPlaceholderText: "Search execution"
    searchableModel: controller.executions
    focus: true
    onCurrentItemChanged: ifCurrentItem('id', (id) => {
      controller.SelectExecution(id);
      execOutputTextArea.closeSearchBar();
    })
    onItemRightClicked: Common.handleRightClick(execList, contextMenu)
    KeyNavigation.right: execOutputTextArea
  }
  Menu {
    id: contextMenu
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
  Rectangle {
    visible: !controller.executionsEmpty
    Layout.fillHeight: true
    width: 1
    color: Theme.colorBgBlack
  }
  ColumnLayout {
    visible: !controller.executionsEmpty
    Layout.fillWidth: true
    spacing: 0
    Cdt.Pane {
      Layout.fillWidth: true
      color: Theme.colorBgMedium
      padding: Theme.basePadding
      ColumnLayout {
        spacing: 0
        width: parent.width
        Cdt.Text {
          Layout.fillWidth: true
          elide: Text.ElideLeft
          text: controller.executionName
        }
        Cdt.Text {
          Layout.fillWidth: true
          text: controller.executionStatus
          color: Theme.colorSubText
        }
      }
    }
    Rectangle {
      Layout.fillWidth: true
      height: 1
      color: Theme.colorBgBlack
    }
    Cdt.ReadOnlyTextArea {
      id: execOutputTextArea
      Layout.fillWidth: true
      Layout.fillHeight: true
      innerPadding: Theme.basePadding
      color: Theme.colorBgDark
      text: controller.executionOutput
      textFormat: TextEdit.RichText
      cursorFollowEnd: true
      searchable: true
    }
  }
}
