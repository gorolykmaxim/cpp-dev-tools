import QtQuick
import Qt.labs.platform
import "." as Cdt
import cdt

Cdt.SearchableTextList {
  Component.onCompleted: documentationSystem.displayDocumentation()
  id: list
  anchors.fill: parent
  searchPlaceholderText: "Search documentation"
  showPlaceholder: documentationSystem.showPlaceholder
  placeholderText: documentationSystem.isLoading ? "Looking for documentation..." : "No documentation found"
  searchableModel: documentationSystem.documents
  focus: true
  onItemRightClicked: contextMenu.open()
  onItemSelected: documentationSystem.openSelectedDocument()
  Menu {
    id: contextMenu
    MenuItem {
      text: "Open"
      onTriggered: documentationSystem.openSelectedDocument()
    }
  }
}
