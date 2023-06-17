import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

ColumnLayout {
  anchors.fill: parent
  spacing: 0
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
  Cdt.FileLinkLookup {
    id: linkLookup
    onRehighlightLine: line => textArea.rehighlightLine(line)
  }
  Cdt.BigTextArea {
    id: textArea
    Layout.fillWidth: true
    Layout.fillHeight: true
    focus: true
    text: controller.executionOutput
    formatter: linkLookup.formatter
    cursorFollowEnd: true
    onPreHighlight: linkLookup.findFileLinks(text)
    onCurrentLineChanged: linkLookup.setCurrentLine(currentLine)
  }
//  Cdt.TextArea {
//    id: execOutputTextArea
//    Layout.fillWidth: true
//    Layout.fillHeight: true
//    focus: true
//    readonly: true
//    innerPadding: Theme.basePadding
//    text: controller.executionOutput
//    formatter: controller.executionFormatter
//    cursorFollowEnd: true
//    searchable: true
//  }
}
