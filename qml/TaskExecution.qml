import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

ColumnLayout {
  anchors.fill: parent
  spacing: 0
  Component.onCompleted: controller.AttachTaskExecutionOutputHighlighter(execOutputTextArea.textDocument)
  TaskExecutionController {
    id: controller
  }
  Cdt.Pane {
    Layout.fillWidth: true
    color: Theme.colorBgMedium
    padding: Theme.basePadding
    RowLayout {
      width: parent.width
      Cdt.Icon {
        icon: controller.executionIcon
        color: controller.executionIconColor
        Layout.margins: Theme.basePadding
      }
      ColumnLayout {
        spacing: 0
        Layout.fillWidth: true
        Layout.fillHeight: true
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
    focus: true
    innerPadding: Theme.basePadding
    color: Theme.colorBgDark
    text: controller.executionOutput
    cursorFollowEnd: true
    searchable: true
  }
}
