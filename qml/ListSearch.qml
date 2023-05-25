import QtQuick
import cdt
import "." as Cdt

Cdt.TextField {
  id: field
  property var list
  property var listModel: null
  text: listModel ? listModel.filter : ""
  signal enterPressed()
  signal ctrlEnterPressed()

  onDisplayTextChanged: {
    if (listModel) {
      listModel.filter = displayText;
    }
  }
  Keys.onPressed: e => {
    switch (e.key) {
      case Qt.Key_Down:
        e.accepted = list.incrementCurrentIndex();
        break;
      case Qt.Key_Up:
        e.accepted = list.decrementCurrentIndex();
        break;
      case Qt.Key_PageUp:
        e.accepted = list.pageUp();
        break;
      case Qt.Key_PageDown:
        e.accepted = list.pageDown();
        break;
      case Qt.Key_Enter:
      case Qt.Key_Return:
        if (e.modifiers & Qt.ControlModifier) {
          field.ctrlEnterPressed()
        } else {
          field.enterPressed()
        }
        e.accepted = true;
        break;
    }
  }
}
