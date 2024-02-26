import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt

SplitView {
  id: root
  property alias sha: showModel.sha
  signal back()
  anchors.fill: parent
  handle: Cdt.SplitViewHandle {
    viewId: "GitShowWindow"
    view: root
  }
  Keys.onEscapePressed: back()
  GitShowModel {
    id: showModel
  }
  SplitView {
    id: sidebar
    SplitView.minimumWidth: gitSystem.commitMessageWidthLong <= root.width * 0.4 ?
                              gitSystem.commitMessageWidthLong :
                              gitSystem.commitMessageWidthShort
    SplitView.fillHeight: true
    orientation: Qt.Vertical
    handle: Cdt.SplitViewHandle {
      viewId: "GitShowSidebar"
      view: sidebar
    }
    ColumnLayout {
      SplitView.fillWidth: true
      SplitView.fillHeight: true
      spacing: 0
      RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.basePadding * 2
        Layout.rightMargin: Theme.basePadding * 2
        Layout.topMargin: Theme.basePadding
        Layout.bottomMargin: Theme.basePadding
        Cdt.IconButton {
          id: backBtn
          buttonIcon: "keyboard_backspace"
          focus: !showModel.hasChanges
          onClicked: root.back()
          KeyNavigation.down: changeList
          KeyNavigation.right: fileDiff
        }
        Cdt.Text {
          Layout.fillWidth: true
          text: "Changes:"
        }
        Cdt.Text {
          text: showModel.stats
          color: Theme.colorPlaceholder
        }
      }
      Cdt.TextList {
        id: changeList
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: showModel
        focus: showModel.hasChanges
        enabled: showModel.hasChanges
        highlightCurrentItemWithoutFocus: false
        KeyNavigation.down: commitMsg
        KeyNavigation.right: fileDiff
      }
    }
    ColumnLayout {
      SplitView.fillWidth: true
      SplitView.minimumHeight: 300
      spacing: 0
      Cdt.BigTextArea {
        id: commitMsg
        text: showModel.rawCommitInfo
        Layout.fillWidth: true
        Layout.fillHeight: true
        enabled: showModel.hasChanges
        KeyNavigation.right: fileDiff
      }
    }
  }
  Cdt.GitDiff {
    id: fileDiff
    SplitView.fillWidth: true
    SplitView.fillHeight: true
    file: showModel.selectedFileName
    rawDiff: showModel.rawDiff
    enabled: showModel.hasChanges
    errorMessage: showModel.diffError
  }
}
