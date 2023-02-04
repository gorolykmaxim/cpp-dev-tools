import QtQuick
import QtQuick.Controls
import Qt.labs.platform

TextField {
  id: textField
  color: colorText
  enabled: visible
  placeholderTextColor: colorSubText
  padding: basePadding
  selectByMouse: true
  selectionColor: colorHighlight
  onPressed: e => {
    if (e.button == Qt.RightButton) {
      contextMenu.open();
    }
  }
  background: Rectangle {
    color: colorBgDark
    border.color: parent.focus ? colorHighlight : colorBgBlack
    border.width: parent.focus ? 2 : 1
    radius: baseRadius
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
  }
}
