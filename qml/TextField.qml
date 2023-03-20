import QtQuick
import QtQuick.Controls
import Qt.labs.platform
import cdt
import "Common.js" as Common

TextField {
  id: textField
  color: Theme.colorText
  enabled: visible
  placeholderTextColor: Theme.colorSubText
  padding: Theme.basePadding
  selectByMouse: true
  selectionColor: Theme.colorHighlight
  onPressed: e => Common.handleRightClick(e, textField, contextMenu)
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
      shortcut: StandardKey.Cut
      enabled: textField.activeFocus
      onTriggered: textField.cut()
    }
    MenuItem {
      text: "Copy"
      shortcut: StandardKey.Copy
      enabled: textField.activeFocus
      onTriggered: textField.copy()
    }
    MenuItem {
      text: "Paste"
      shortcut: StandardKey.Paste
      enabled: textField.activeFocus
      onTriggered: textField.paste()
    }
    MenuSeparator {}
    MenuItem {
      text: "Select All"
      shortcut: StandardKey.SelectAll
      enabled: textField.activeFocus
      onTriggered: textField.selectAll()
    }
  }
}
