import QtQuick
import Qt.labs.platform
import "." as Cdt
import cdt

Loader {
  id: root
  anchors.fill: parent
  focus: true
  sourceComponent: listView
  GitBranchListController {
    id: controller
  }
  Component {
    id: listView
    Cdt.SearchableTextList {
      id: list
      anchors.fill: parent
      searchPlaceholderText: "Search branch"
      searchableModel: controller.branches
      focus: true
      onItemRightClicked: contextMenu.open()
      onItemSelected: gitSystem.checkoutBranch(controller.selectedBranchIndex)
      Component.onCompleted: controller.displayList()
      Menu {
        id: contextMenu
        MenuItem {
          text: "Checkout"
          enabled: list.activeFocus
          onTriggered: gitSystem.checkoutBranch(controller.selectedBranchIndex)
        }
        MenuItem {
          text: "New Branch"
          shortcut: gSC("GitBranchList", "New Branch")
          enabled: list.activeFocus
          onTriggered: root.sourceComponent = createView
        }
        MenuItem {
          text: "Merge Into Current"
          shortcut: gSC("GitBranchList", "Merge Into Current")
          enabled: list.activeFocus
          onTriggered: gitSystem.mergeBranchIntoCurrent(controller.selectedBranchIndex)
        }
        MenuItem {
          text: "Delete"
          shortcut: gSC("GitBranchList", "Delete")
          enabled: list.activeFocus && controller.isLocalBranchSelected
          onTriggered: gitSystem.deleteBranch(controller.selectedBranchIndex, false)
        }
        MenuItem {
          text: "Force Delete"
          shortcut: gSC("GitBranchList", "Force Delete")
          enabled: list.activeFocus && controller.isLocalBranchSelected
          onTriggered: gitSystem.deleteBranch(controller.selectedBranchIndex, true)
        }
      }
    }
  }
  Component {
    id: createView
    Cdt.NewGitBranch {
      branchBasis: controller.selectedBranchName
      onBack: root.sourceComponent = listView
    }
  }
}
