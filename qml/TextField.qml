import QtQuick
import QtQuick.Controls as QtQuick
import Qt.labs.platform
import cdt
import "Common.js" as Common

QtQuick.TextField {
  id: textField
  color: Theme.colorText
  enabled: visible
  placeholderTextColor: Theme.colorSubText
  padding: Theme.basePadding
  selectByMouse: true
  selectionColor: Theme.colorHighlight
  onPressed: e => Common.handleRightClick(textField, contextMenu, e)
  background: Rectangle {
    color: Theme.colorBgDark
    border.color: parent.activeFocus ? Theme.colorHighlight : Theme.colorBgBlack
    border.width: parent.activeFocus ? 2 : 1
    radius: Theme.baseRadius
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Cut"
      shortcut: "Ctrl+X"
      enabled: textField.activeFocus
      onTriggered: textField.cut()
    }
    MenuItem {
      text: "Copy"
      shortcut: "Ctrl+C"
      enabled: textField.activeFocus
      onTriggered: textField.copy()
    }
    MenuItem {
      text: "Paste"
      shortcut: "Ctrl+V"
      enabled: textField.activeFocus
      onTriggered: textField.paste()
    }
    MenuSeparator {}
    MenuItem {
      text: "Select All"
      shortcut: "Ctrl+A"
      enabled: textField.activeFocus
      onTriggered: textField.selectAll()
    }
  }
}
