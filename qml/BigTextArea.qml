import QtQuick
import QtQuick.Controls
import Qt.labs.platform
import "." as Cdt
import cdt

Cdt.Pane {
  id: root
  property alias text: textModel.text
  property bool monoFont: true
  property alias cursorFollowEnd: textModel.cursorFollowEnd
  color: Theme.colorBgDark
  TextAreaModel {
    id: textModel
    onGoToLine: function(line) {
      listView.currentIndex = line;
    }
  }
  ListView {
    id: listView
    anchors.fill: parent
    focus: true
    clip: true
    boundsBehavior: ListView.StopAtBounds
    boundsMovement: ListView.StopAtBounds
    highlightMoveDuration: 100
    ScrollBar.vertical: ScrollBar {}
    model: textModel
    Keys.onPressed: function(e) {
      if (e.matches(StandardKey.MoveToStartOfDocument)) {
        currentIndex = 0;
      } else if (e.matches(StandardKey.MoveToEndOfDocument)) {
        currentIndex = count -1;
      } else if (e.key === Qt.Key_PageUp) {
        currentIndex = Math.max(currentIndex - 10, 0);
      } else if (e.key === Qt.Key_PageDown) {
        currentIndex = Math.min(currentIndex + 10, count - 1);
      }
    }
    onCurrentIndexChanged: textModel.currentLine = currentIndex
    delegate: Cdt.Pane {
      property int itemIndex: index
      width: listView.width
      color: ListView.isCurrentItem ? Theme.colorBgMedium : Theme.colorBgDark
      TextEdit {
        anchors.fill: parent
        selectByMouse: true
        selectByKeyboard: true
        readOnly: true
        enabled: root.enabled
        selectionColor: Theme.colorHighlight
        selectedTextColor: Theme.colorBgBlack
        text: model.text
        textFormat: TextEdit.PlainText
        color: root.enabled ? Theme.colorText : Theme.colorSubText
        font.family: root.monoFont ? monoFontFamily : null
        font.pointSize: root.monoFont ? monoFontSize : -1
        renderType: Text.NativeRendering
        wrapMode: Text.WordWrap
        onSelectedTextChanged: textModel.selectInline(itemIndex, selectionStart, selectionEnd)
        Keys.onPressed: function(e) {
          if (e.matches(StandardKey.Copy)) {
            copyMenuItem.triggered();
            e.accepted = true;
          }
        }
        MouseArea {
          anchors.fill: parent
          acceptedButtons: Qt.LeftButton | Qt.RightButton
          cursorShape: Qt.IBeamCursor
          onPressed: function(event) {
            listView.currentIndex = itemIndex;
            if (event.button === Qt.RightButton) {
              contextMenu.open();
              event.accepted = true;
            } else {
              event.accepted = false;
            }
          }
        }
      }
    }
    Menu {
      id: contextMenu
      MenuItem {
        id: copyMenuItem
        text: "Copy"
        shortcut: "Ctrl+C"
        enabled: listView.activeFocus
        onTriggered: textModel.copySelection()
      }
    }
  }
}
