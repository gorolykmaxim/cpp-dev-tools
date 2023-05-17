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
      onItemSelected: controller.checkoutSelected()
      Component.onCompleted: controller.displayList()
      Menu {
        id: contextMenu
        MenuItem {
          text: "Checkout"
          enabled: list.activeFocus
          onTriggered: controller.checkoutSelected()
        }
        MenuItem {
          text: "New Branch"
          shortcut: "Alt+N"
          enabled: list.activeFocus
          onTriggered: root.sourceComponent = createView
        }
        MenuItem {
          text: "Delete"
          shortcut: "Alt+D"
          enabled: list.activeFocus && controller.isLocalBranchSelected
          onTriggered: controller.deleteSelected(false)
        }
        MenuItem {
          text: "Force Delete"
          shortcut: "Alt+Shift+D"
          enabled: list.activeFocus && controller.isLocalBranchSelected
          onTriggered: controller.deleteSelected(true)
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
