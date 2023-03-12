import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "." as Cdt
import cdt

RowLayout {
  Component.onCompleted: execList.forceActiveFocus()
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
    searchPlaceholderText: "Search execution"
    searchableModel: controller.executions
    onCurrentItemChanged: ifCurrentItem('id', controller.SelectExecution)
    KeyNavigation.right: execOutputTextArea
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
          text: controller.executionCommand
        }
        Cdt.Text {
          Layout.fillWidth: true
          text: controller.executionStatus
          color: Theme.colorSubText
        }
      }
    }
    Cdt.Pane {
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: Theme.colorBgDark
      Cdt.ReadOnlyTextArea {
        id: execOutputTextArea
        anchors.fill: parent
        innerPadding: Theme.basePadding
        textData: controller.executionOutput
        textFormat: TextEdit.RichText
        cursorFollowEnd: true
        navigationLeft: execList
      }
    }
  }
}
