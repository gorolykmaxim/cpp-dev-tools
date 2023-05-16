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
  GitBranchListController {
    id: controller
  }
  Menu {
    id: contextMenu
  }
}
