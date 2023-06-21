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
          shortcut: "Alt+Shift+D"
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
        placeholderText: "Commit Message"
        focus: controller.hasChanges
        enabled: controller.hasChanges
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
    file: controller.files.selectedFileName
    rawDiff: controller.rawDiff
    enabled: controller.hasChanges
    additionalMenuItems: [
      {
        text: "Rollback Chunk",
        shortcut: "Ctrl+Alt+Z",
        callback: () => controller.rollbackChunk(currentChunk, chunkCount),
      }
    ]
  }
//  Cdt.TextArea {
//    id: fileDiff
//    SplitView.fillWidth: true
//    SplitView.fillHeight: true
//    centeredPlaceholderText: controller.diffError
//    centeredPlaceholderTextColor: controller.diffError ? "red" : null
//    readonly: true
//    wrapMode: TextEdit.NoWrap
//    enabled: controller.hasChanges
//    text: controller.diff
//    formatter: controller.formatter
//    detectFileLinks: false
//    searchable: true
//    onWidthChanged: controller.resizeDiff(width)
//    disableLoadingPlaceholder: true
//    KeyNavigation.left: changeList
//    menuItems: [
//      MenuSeparator {},
//      MenuItem {
//        text: "Toggle Unified Diff"
//        enabled: fileDiff.activeFocus && controller.isSelectedFileModified
//        shortcut: "Alt+U"
//        onTriggered: controller.toggleUnifiedDiff()
//      },
//      MenuItem {
//        text: "Rollback Chunk"
//        enabled: fileDiff.activeFocus
//        shortcut: "Ctrl+Alt+Z"
//        onTriggered: controller.rollbackChunk(fileDiff.effectiveCursorPosition)
//      },
//      MenuItem {
//        text: "Open Chunk In Editor"
//        enabled: fileDiff.activeFocus
//        shortcut: "Ctrl+Shift+O"
//        onTriggered: controller.openChunkInEditor(fileDiff.effectiveCursorPosition)
//      }
//    ]
//  }
}
