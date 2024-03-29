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
          color: Theme.colorPlaceholder
        }
      }
    }
  }
  Rectangle {
    Layout.fillWidth: true
    height: 1
    color: Theme.colorBorder
  }
  Cdt.FileLinkLookup {
    id: linkLookup
    onRehighlightLine: line => textArea.rehighlightLine(line)
    onLinkInLineSelected: line => textArea.goToLine(line)
  }
  Cdt.BigTextArea {
    id: textArea
    Layout.fillWidth: true
    Layout.fillHeight: true
    focus: true
    text: controller.executionOutput
    formatters: [controller.executionFormatter, linkLookup.formatter]
    cursorFollowEnd: true
    onPreHighlight: linkLookup.findFileLinks(text)
    onCurrentLineChanged: linkLookup.setCurrentLine(currentLine)
    additionalMenuItems: linkLookup.menuItems
  }
}
