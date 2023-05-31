import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt

SplitView {
  id: root
  anchors.fill: parent
  handle: Cdt.SplitViewHandle {
    viewId: "GitCommitWindow"
    view: root
  }
  GitCommitController {
    id: controller
    onCommitMessageChanged: txt => commitMsg.setText(txt)
  }
  Keys.onPressed: function(e) {
    if (e.key === Qt.Key_F5) {
      controller.findChangedFiles()
    }
  }
  SplitView {
    id: sidebar
    SplitView.minimumWidth: controller.sidebarWidth
    SplitView.fillHeight: true
    orientation: Qt.Vertical
    handle: Cdt.SplitViewHandle {
      viewId: "GitCommitSidebar"
      view: sidebar
    }
    ColumnLayout {
      SplitView.fillWidth: true
      SplitView.fillHeight: true
      spacing: 0
      Cdt.Pane {
        Layout.fillWidth: true
        color: Theme.colorBgMedium
        focus: !controller.hasChanges
        KeyNavigation.down: changeList
        KeyNavigation.right: fileDiff
        RowLayout {
          anchors.fill: parent
          Cdt.Text {
            text: "Changed Files"
            Layout.fillWidth: true
            padding: Theme.basePadding
          }
          Cdt.IconButton {
            buttonIcon: "loop"
            onClicked: controller.findChangedFiles()
            focus: true
          }
        }
      }
      Cdt.TextList {
        id: changeList
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: controller.files
        enabled: controller.hasChanges
        highlightCurrentItemWithoutFocus: false
        onItemRightClicked: contextMenu.open()
        Keys.onEnterPressed: controller.toggleStagedSelectedFile()
        Keys.onReturnPressed: controller.toggleStagedSelectedFile()
        KeyNavigation.down: commitMsg
        KeyNavigation.right: fileDiff
        Menu {
          id: contextMenu
          MenuItem {
            enabled: changeList.activeFocus
            text: "Toggle Staged"
            onTriggered: controller.toggleStagedSelectedFile()
          }
          MenuItem {
            enabled: changeList.activeFocus
            text: "Reset"
            shortcut: "Alt+Shift+D"
            onTriggered: controller.resetSelectedFile()
          }
        }
      }
    }
    ColumnLayout {
      SplitView.fillWidth: true
      SplitView.minimumHeight: 300
      spacing: 0
      Cdt.TextArea {
        id: commitMsg
        Layout.fillWidth: true
        Layout.fillHeight: true
        placeholderText: "Commit Message"
        focus: controller.hasChanges
        innerPadding: Theme.basePadding
        enabled: controller.hasChanges
        detectFileLinks: false
        searchable: true
        KeyNavigation.down: commitButtons
        KeyNavigation.right: fileDiff
      }
      Cdt.Pane {
        id: commitButtons
        Layout.fillWidth: true
        color: Theme.colorBgMedium
        padding: Theme.basePadding
        enabled: controller.hasChanges
        KeyNavigation.right: fileDiff
        RowLayout {
          anchors.fill: parent
          spacing: Theme.basePadding
          Cdt.Button {
            text: "Commit All"
            focus: true
            onClicked: controller.commit(commitMsg.displayText, true, amendCheckBox.checked)
            KeyNavigation.right: commitBtn
          }
          Cdt.Button {
            id: commitBtn
            text: "Commit"
            onClicked: controller.commit(commitMsg.displayText, false, amendCheckBox.checked)
            KeyNavigation.right: amendCheckBox
          }
          Cdt.CheckBox {
            id: amendCheckBox
            text: "Amend"
            onCheckedChanged: checked && controller.loadLastCommitMessage()
            Layout.fillWidth: true
          }
        }
      }
    }
  }
  RowLayout {
    SplitView.fillWidth: true
    SplitView.fillHeight: true
    Cdt.PlaceholderText {
      Layout.fillWidth: true
      Layout.fillHeight: true
      text: controller.diffError
      visible: controller.diffError
      textColor: "red"
    }
    Cdt.TextArea {
      id: fileDiff
      Layout.fillWidth: true
      Layout.fillHeight: true
      visible: !controller.diffError
      readonly: true
      wrapMode: TextEdit.NoWrap
      enabled: controller.hasChanges
      text: controller.diff
      formatter: controller.formatter
      detectFileLinks: false
      searchable: true
      onWidthChanged: controller.resizeDiff(width)
      disableLoadingPlaceholder: true
      KeyNavigation.left: changeList
      menuItems: [
        MenuSeparator {},
        MenuItem {
          text: "Toggle Unified Diff"
          enabled: fileDiff.activeFocus && controller.isSelectedFileModified
          shortcut: "Alt+U"
          onTriggered: controller.toggleUnifiedDiff()
        }
      ]
    }
  }
}
