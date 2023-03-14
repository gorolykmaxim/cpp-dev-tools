import QtQuick
import QtQuick.Controls
import Qt.labs.platform
import cdt

TextField {
  id: textField
  color: Theme.colorText
  enabled: visible
  placeholderTextColor: Theme.colorSubText
  padding: Theme.basePadding
  selectByMouse: true
  selectionColor: Theme.colorHighlight
  onPressed: e => {
    if (e.button == Qt.RightButton) {
      contextMenu.open();
    }
  }
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
      onTriggered: textField.cut()
    }
    MenuItem {
      text: "Copy"
      shortcut: StandardKey.Copy
      onTriggered: textField.copy()
    }
    MenuItem {
      text: "Paste"
      shortcut: StandardKey.Paste
      onTriggered: textField.paste()
    }
    MenuSeparator {}
    MenuItem {
      text: "Select All"
      shortcut: StandardKey.SelectAll
      onTriggered: textField.selectAll()
    }
  }
}
