import cdt
import QtQuick
import Qt.labs.platform

FileLinkLookupController {
  property list<QtObject> menuItems: [
    MenuSeparator {},
    MenuItem {
      text: "Open File In Editor"
      shortcut: "Ctrl+O"
      onTriggered: openCurrentFileLink()
    }
  ]
}
