import QtQuick
import Qt.labs.platform
import "." as Cdt
import cdt

Cdt.SearchableTextList {
  id: list
  anchors.fill: parent
  searchPlaceholderText: "Search branch"
  searchableModel: controller.branches
  focus: true
  onItemRightClicked: contextMenu.open()
  onItemSelected: controller.checkoutSelected()
  GitBranchListController {
    id: controller
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Checkout"
      enabled: list.activeFocus
      onTriggered: controller.checkoutSelected()
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
