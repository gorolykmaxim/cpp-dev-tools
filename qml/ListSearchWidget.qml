import QtQuick
import "Common.js" as Cmn

TextFieldWidget {
  property var list
  property var changeEventType
  property var enterEventType
  property var ctrlEnterEventType
  property var listIdFieldName

  onDisplayTextChanged: core.OnAction(changeEventType, [displayText])
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
        if (e.modifiers & Qt.ControlModifier && ctrlEnterEventType) {
          core.OnAction(ctrlEnterEventType, []);
        } else if (enterEventType) {
          Cmn.onListAction(list, enterEventType, listIdFieldName);
        }
        break;
    }
  }
}
