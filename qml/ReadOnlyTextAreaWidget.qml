import QtQuick
import QtQuick.Controls
import Qt.labs.platform

TextArea {
  id: textArea
  property var allowedKeys: new Set([
    Qt.Key_Left,
    Qt.Key_Right,
    Qt.Key_Up,
    Qt.Key_Down,
    Qt.Key_Home,
    Qt.Key_End,
  ])
  selectByMouse: true
  color: colorText
  background: Rectangle {
    color: "transparent"
  }
  wrapMode: TextArea.WordWrap
  // Make text area effectively readOnly but don't hide the cursor and
  // allow navigating it using the cursor
  Keys.onPressed: event => {
    if (!allowedKeys.has(event.key) &&
        !event.matches(StandardKey.Copy) &&
        !event.matches(StandardKey.SelectAll)) {
      event.accepted = true;
    }
  }
  onPressed: event => {
    if (event.button == Qt.RightButton) {
      contextMenu.open();
    }
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Copy"
      shortcut: StandardKey.Copy
      onTriggered: textArea.copy()
    }
  }
}
