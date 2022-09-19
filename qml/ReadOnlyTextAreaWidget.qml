import QtQuick
import QtQuick.Controls

TextArea {
  background: Rectangle {
    color: "transparent"
  }
  wrapMode: TextArea.WordWrap
  // Make text area effectively readOnly but don't hide the cursor and
  // allow navigating it using the cursor
  Keys.onPressed: event => {
    if (event.key != Qt.Key_Left && event.key != Qt.Key_Right &&
        event.key != Qt.Key_Up && event.key != Qt.Key_Down) {
      event.accepted = true;
    }
  }
}
