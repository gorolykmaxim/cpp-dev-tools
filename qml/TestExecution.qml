import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import cdt
import "." as Cdt

ColumnLayout {
  required property QtObject testModel
  required property string taskName
  anchors.fill: parent
  spacing: 0
  RowLayout {
    Layout.margins: Theme.basePadding
    Layout.bottomMargin: 0
    spacing: Theme.basePadding
    Cdt.Text {
      text: taskName
    }
    Cdt.Text {
      text: testModel.status
      color: Theme.colorPlaceholder
    }
  }
  Rectangle {
    Layout.fillWidth: true
    Layout.margins: Theme.basePadding
    color: Theme.colorBorder
    height: Theme.basePadding
    radius: Theme.baseRadius
    Rectangle {
      anchors.top: parent.top
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      width: parent.width * testModel.progress
      radius: Theme.baseRadius
      color: testModel.progressBarColor
    }
  }
  Rectangle {
    Layout.fillWidth: true
    height: 1
    color: Theme.colorBorder
  }
  SplitView {
    id: splitView
    Layout.fillWidth: true
    Layout.fillHeight: true
    handle: Cdt.SplitViewHandle {
      viewId: "TestExecutionWindow"
      view: splitView
    }
    Cdt.SearchableTextList {
      id: testList
      SplitView.minimumWidth: 300
      SplitView.fillHeight: true
      searchPlaceholderText: "Search test"
      focus: true
      searchableModel: testModel
      KeyNavigation.right: testOutput
      Menu {
        id: contextMenu
        MenuItem {
          text: "Re-Run"
          enabled: testList.activeFocus
          shortcut: "Alt+Shift+R"
          onTriggered: testModel.rerunSelectedTest(false)
        }
        MenuItem {
          text: "Re-Run Until Fails"
          enabled: testList.activeFocus
          shortcut: "Ctrl+Alt+Shift+R"
          onTriggered: testModel.rerunSelectedTest(true)
        }
      }
    }
    Cdt.FileLinkLookup {
      id: linkLookup
      onRehighlightLine: line => testOutput.rehighlightLine(line)
      onLinkInLineSelected: line => testOutput.goToLine(line)
    }
    Cdt.BigTextArea {
      id: testOutput
      SplitView.fillWidth: true
      SplitView.fillHeight: true
      text: testModel.selectedTestOutput
      formatters: [linkLookup.formatter]
      cursorFollowEnd: true
      onPreHighlight: linkLookup.findFileLinks(text)
      onCurrentLineChanged: linkLookup.setCurrentLine(currentLine)
      additionalMenuItems: linkLookup.menuItems
    }
  }
}
