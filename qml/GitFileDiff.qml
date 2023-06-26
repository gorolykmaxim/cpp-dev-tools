import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import cdt
import "." as Cdt

ColumnLayout {
  anchors.fill: parent
  GitFileDiffController {
    id: controller
  }
  RowLayout {
    Layout.fillWidth: true
    spacing: Theme.basePadding
    Layout.margins: Theme.basePadding
    Cdt.TextField {
      id: branchTextField
      text: controller.branchesToCompare
      onDisplayTextChanged: controller.setBranchesToCompare(displayText)
      placeholderText: "branch1..branch2"
      focus: true
      Layout.minimumWidth: 200
      KeyNavigation.right: filePathTextField
      KeyNavigation.down: gitDiffFiles.visible ? gitDiffFiles : gitDiff
      Keys.onEnterPressed: controller.showDiff()
      Keys.onReturnPressed: controller.showDiff()
    }
    Cdt.TextField {
      id: filePathTextField
      text: controller.filePath
      onDisplayTextChanged: controller.setFilePath(displayText)
      placeholderText: "File path"
      Layout.fillWidth: true
      KeyNavigation.right: compareBtn
      KeyNavigation.down: gitDiffFiles.visible ? gitDiffFiles : gitDiff
      Keys.onEnterPressed: controller.showDiff()
      Keys.onReturnPressed: controller.showDiff()
    }
    Cdt.Button {
      id: compareBtn
      text: "Compare"
      onClicked: controller.showDiff()
      KeyNavigation.down: gitDiffFiles.visible ? gitDiffFiles : gitDiff
    }
  }
  SplitView {
    id: mainView
    Layout.fillWidth: true
    Layout.fillHeight: true
    handle: Cdt.SplitViewHandle {
      viewId: "GitFileDiffWindow"
      view: mainView
    }
    Cdt.TextList {
      id: gitDiffFiles
      SplitView.minimumWidth: 200
      SplitView.fillHeight: true
      highlightCurrentItemWithoutFocus: false
      model: controller.files
      visible: controller.files.moreThanOne
      KeyNavigation.right: gitDiff
    }
    Cdt.GitDiff {
      id: gitDiff
      SplitView.fillWidth: true
      SplitView.fillHeight: true
      rawDiff: controller.rawDiff
      enabled: controller.rawDiff
      file: controller.files.selected
      errorMessage: controller.diffError
    }
  }
}
