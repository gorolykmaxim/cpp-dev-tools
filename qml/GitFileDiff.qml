import QtQuick
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
      onDisplayTextChanged: controller.setBranchesToCompare(displayText)
      placeholderText: "branch1..branch2"
      focus: true
      Layout.minimumWidth: 200
      KeyNavigation.right: filePathTextField
      KeyNavigation.down: gitDiff
      Keys.onEnterPressed: controller.showDiff()
      Keys.onReturnPressed: controller.showDiff()
    }
    Cdt.TextField {
      id: filePathTextField
      onDisplayTextChanged: controller.setFilePath(displayText)
      placeholderText: "File path"
      Layout.fillWidth: true
      KeyNavigation.right: compareBtn
      KeyNavigation.down: gitDiff
      Keys.onEnterPressed: controller.showDiff()
      Keys.onReturnPressed: controller.showDiff()
    }
    Cdt.Button {
      id: compareBtn
      text: "Compare"
      onClicked: controller.showDiff()
      KeyNavigation.down: gitDiff
    }
  }
  Cdt.GitDiff {
    id: gitDiff
    rawDiff: controller.rawDiff
    enabled: controller.rawDiff
    file: controller.file
    errorMessage: controller.diffError
    Layout.fillWidth: true
    Layout.fillHeight: true
  }
}
