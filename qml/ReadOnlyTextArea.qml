import QtQuick
import QtQuick.Controls
import Qt.labs.platform
import cdt

ScrollView {
  id: root
  property var allowedKeys: new Set([
    Qt.Key_Left,
    Qt.Key_Right,
    Qt.Key_Up,
    Qt.Key_Down,
    Qt.Key_Home,
    Qt.Key_End,
  ])
  property bool cursorFollowEnd: false
  property string textData: ""
  property var textFormat: TextEdit.PlainText
  property real innerPadding: 0
  property var navigationUp: null
  property var navigationDown: null
  property var navigationLeft: null
  property var navigationRight: null
  onTextDataChanged: {
    textArea.text = textData;
    if (!cursorFollowEnd) {
      return;
    }
    textArea.cursorPosition = textArea.length;
  }
  onActiveFocusChanged: {
    if (activeFocus) {
      textArea.forceActiveFocus();
    }
  }
  TextArea {
    id: textArea
    selectByMouse: true
    selectionColor: Theme.colorHighlight
    color: Theme.colorText
    leftPadding: root.innerPadding
    rightPadding: root.innerPadding
    topPadding: root.innerPadding
    bottomPadding: root.innerPadding
    textFormat: root.textFormat
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
    KeyNavigation.up: navigationUp
    KeyNavigation.down: navigationDown
    KeyNavigation.left: navigationLeft
    KeyNavigation.right: navigationRight
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
}
