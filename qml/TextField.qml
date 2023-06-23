import QtQuick
import QtQuick.Controls as QtQuick
import Qt.labs.platform
import cdt
import "Common.js" as Common

QtQuick.TextField {
  id: textField
  color: Theme.colorText
  enabled: visible
  placeholderTextColor: Theme.colorPlaceholder
  padding: Theme.basePadding
  selectByMouse: true
  selectedTextColor: Theme.colorText
  selectionColor: Theme.colorTextSelection
  onPressed: e => Common.handleRightClick(textField, contextMenu, e)
  renderType: Text.NativeRendering
  background: Rectangle {
    color: parent.activeFocus ? Theme.colorBackground : Theme.colorBorder
    border.color: Theme.colorBorder
    border.width: 1
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
    MenuItem {
      text: "Select Word"
      shortcut: "Ctrl+D"
      enabled: textField.activeFocus
      onTriggered: textField.selectWord()
    }
  }
}
