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
    onCommitMessageChanged: txt => commitMsg.text = txt
  }
  Connections {
    target: appWindow
    function onActiveChanged() {
      if (appWindow.active) {
        controller.findChangedFiles();
      }
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
    Cdt.TextList {
      id: changeList
      SplitView.fillWidth: true
      SplitView.fillHeight: true
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
          shortcut: "Ctrl+Alt+Z"
          onTriggered: controller.resetSelectedFile()
        }
      }
    }
    ColumnLayout {
      SplitView.fillWidth: true
      SplitView.minimumHeight: 300
      spacing: 0
      Cdt.SmallTextArea {
        id: commitMsg
        Layout.fillWidth: true
        Layout.fillHeight: true
        formatter: controller.formatter
        placeholderText: "Commit Message"
        focus: controller.hasChanges
        enabled: controller.hasChanges
        KeyNavigation.down: commitButtons
        KeyNavigation.right: fileDiff
      }
      Cdt.Pane {
        id: commitButtons
        Layout.fillWidth: true
        padding: Theme.basePadding
        enabled: controller.hasChanges
        KeyNavigation.right: fileDiff
        RowLayout {
          anchors.fill: parent
          spacing: Theme.basePadding
          Cdt.Button {
            text: "Commit All"
            focus: true
            onClicked: controller.commit(commitMsg.text, true, amendCheckBox.checked)
            KeyNavigation.right: commitBtn
          }
          Cdt.Button {
            id: commitBtn
            text: "Commit"
            onClicked: controller.commit(commitMsg.text, false, amendCheckBox.checked)
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
  Cdt.GitDiff {
    id: fileDiff
    SplitView.fillWidth: true
    SplitView.fillHeight: true
    file: controller.files.selectedFileName
    rawDiff: controller.rawDiff
    enabled: controller.hasChanges
    errorMessage: controller.diffError
    KeyNavigation.left: changeList
    additionalMenuItems: [
      {
        text: "Rollback Chunk",
        shortcut: "Ctrl+Alt+Z",
        callback: () => controller.rollbackChunk(currentChunk, chunkCount),
      }
    ]
  }
}
