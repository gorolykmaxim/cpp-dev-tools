import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt

Cdt.Pane {
  id: root
  property alias text: textModel.text
  property bool monoFont: true
  property alias cursorFollowEnd: textModel.cursorFollowEnd
  property alias cursorPosition: textModel.cursorPosition
  property bool displayLineNumbers: false
  color: Theme.colorBgDark
  TextAreaModel {
    id: textModel
    onGoToLine: function(line) {
      listView.currentIndex = line;
      listView.positionViewAtIndex(listView.currentIndex, ListView.Center);
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
      RowLayout {
        anchors.fill: parent
        spacing: 0
        Cdt.Text {
          visible: root.displayLineNumbers
          text: itemIndex + 1
          color: Theme.colorSubText
          Layout.fillHeight: true
          Layout.minimumWidth: textModel.lineNumberMaxWidth
          Layout.leftMargin: Theme.basePadding
          Layout.rightMargin: Theme.basePadding
          horizontalAlignment: Text.AlignRight
          verticalAlignment: Text.AlignTop
          font.family: root.monoFont ? monoFontFamily : null
          font.pointSize: root.monoFont ? monoFontSize : -1
        }
        TextEdit {
          id: textEdit
          property bool ignoreSelect: false
          Layout.fillWidth: true
          activeFocusOnPress: false
          selectByMouse: true
          selectByKeyboard: true
          readOnly: true
          enabled: root.enabled
          selectionColor: "transparent"
          selectedTextColor: "transparent"
          text: model.text
          textFormat: TextEdit.PlainText
          color: root.enabled ? Theme.colorText : Theme.colorSubText
          font.family: root.monoFont ? monoFontFamily : null
          font.pointSize: root.monoFont ? monoFontSize : -1
          renderType: Text.NativeRendering
          wrapMode: Text.WordWrap
          onSelectedTextChanged: {
            if (ignoreSelect) {
              return;
            }
            ignoreSelect = true;
            textModel.selectInline(itemIndex, selectionStart, selectionEnd);
            ignoreSelect = false;
          }
          Connections {
            target: textModel
            function onRehighlightLines(first, last) {
              if (itemIndex >= first && itemIndex <= last) {
                highlighter.rehighlight();
              }
            }
          }
          LineHighlighter {
            id: highlighter
            document: textEdit.textDocument
            formatters: textModel.formatters
            lineNumber: itemIndex
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
              } else if (event.modifiers & Qt.ControlModifier) {
                textModel.selectLine(itemIndex);
                event.accepted = true;
              } else {
                event.accepted = false;
              }
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
      MenuItem {
        text: "Toggle Select"
        shortcut: "Ctrl+S"
        enabled: listView.activeFocus
        onTriggered: textModel.selectLine(listView.currentIndex)
      }
      MenuItem {
        id: selectAllMenuItem
        text: "Select All"
        shortcut: "Ctrl+A"
        enabled: listView.activeFocus
        onTriggered: textModel.selectAll()
      }
      MenuItem {
        text: "Clear Selection"
        shortcut: "Esc"
        enabled: listView.activeFocus
        onTriggered: textModel.resetSelection()
      }
    }
  }
}
