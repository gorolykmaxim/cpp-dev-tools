import QtQuick
import QtQuick.Controls

TextField {
  id: textField
  color: colorText
  padding: basePadding
  selectByMouse: true
  selectionColor: colorHighlight
  onPressed: e => {
    if (e.button == Qt.RightButton) {
      contextMenu.popup();
    }
  }
  background: Rectangle {
    color: colorBgDark
    border.color: parent.focus ? colorHighlight : colorBgBlack
    border.width: parent.focus ? 2 : 1
    radius: baseRadius
  }
  ContextMenuWidget {
    id: contextMenu
    ContextMenuItemWidget {
      text: "Cut"
      onTriggered: textField.cut()
    }
    ContextMenuItemWidget {
      text: "Copy"
      onTriggered: textField.copy()
    }
    ContextMenuItemWidget {
      text: "Paste"
      onTriggered: textField.paste()
    }
  }
}
