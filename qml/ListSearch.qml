import QtQuick
import cdt
import "." as Cdt

Cdt.TextField {
  id: field
  property var list
  signal enterPressed()
  signal ctrlEnterPressed()

  Keys.onPressed: e => {
    switch (e.key) {
      case Qt.Key_Down:
        list.incrementCurrentIndex();
        break;
      case Qt.Key_Up:
        list.decrementCurrentIndex();
        break;
      case Qt.Key_PageUp:
        for (let i = 0; i < 10; i++) {
          list.decrementCurrentIndex();
        }
        break;
      case Qt.Key_PageDown:
        for (let i = 0; i < 10; i++) {
          list.incrementCurrentIndex();
        }
        break;
      case Qt.Key_Enter:
      case Qt.Key_Return:
        if (e.modifiers & Qt.ControlModifier) {
          field.ctrlEnterPressed()
        } else {
          field.enterPressed()
        }
        break;
    }
  }
}
