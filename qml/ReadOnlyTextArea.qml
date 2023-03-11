import QtQuick
import QtQuick.Controls
import Qt.labs.platform
import cdt

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
  property int selectionStart: 0 // TODO: remove
  property int selectionEnd: 0 // TODO: remove
  property bool cursorFollowEnd: false
  property string textData: ""
  selectByMouse: true
  selectionColor: Theme.colorHighlight
  color: Theme.colorText
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
  onSelectionStartChanged: textArea.select(selectionStart, selectionEnd);
  onSelectionEndChanged: textArea.select(selectionStart, selectionEnd);
  onTextDataChanged: {
    text = textData;
    textArea.select(selectionStart, selectionEnd);
    if (!cursorFollowEnd || selectionStart > 0 || selectionEnd > 0) {
      return;
    }
    cursorPosition = length;
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Copy"
      shortcut: StandardKey.Copy
      onTriggered: textArea.copy()
    }
    MenuSeparator {}
    MenuItem {
      text: "Select All"
      shortcut: StandardKey.SelectAll
      onTriggered: textArea.selectAll()
    }
  }
}
