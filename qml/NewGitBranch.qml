import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

Cdt.Pane {
  property alias branchBasis: controller.branchBasis
  signal back()
  anchors.fill: parent
  focus: true
  color: Theme.colorBgMedium
  Keys.onEscapePressed: back()
  NewGitBranchController {
    id: controller
    onSuccess: back()
  }
  ColumnLayout {
    anchors.centerIn: parent
    width: Theme.centeredViewWidth
    spacing: Theme.basePadding
    Cdt.Text {
      text: "Create new branch based on <b>" + controller.branchBasis + "</b>"
    }
    RowLayout {
      Layout.fillWidth: true
      spacing: Theme.basePadding
      Cdt.TextField {
        id: input
        Layout.fillWidth: true
        focus: true
        placeholderText: "Branch name"
        Keys.onEnterPressed: controller.createBranch(input.displayText)
        Keys.onReturnPressed: controller.createBranch(input.displayText)
        KeyNavigation.right: createBtn
      }
      Cdt.Button {
        id: createBtn
        text: "Create"
        onClicked: controller.createBranch(input.displayText)
        KeyNavigation.right: backBtn
      }
      Cdt.Button {
        id: backBtn
        text: "Back"
        onClicked: back()
      }
    }
    Cdt.Text {
      id: errorText
      color: "red"
      text: controller.error
    }
  }
}
