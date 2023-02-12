import QtQuick
import QtQuick.Controls

TextArea {
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
    if (!allowedKeys.has(event.key)) {
      event.accepted = true;
    }
  }
}
