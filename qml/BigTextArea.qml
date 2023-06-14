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
  Keys.onEscapePressed: function(e) {
    if (textModel.resetSelection()) {
      e.accepted = true;
    } else if (searchBar.close()) {
      e.accepted = true;
    }
  }
  ColumnLayout {
    anchors.fill: parent
    spacing: 0
    Cdt.TextSearchBar {
      id: searchBar
      text: textModel.text
      Layout.fillWidth: true
      readOnly: true
      KeyNavigation.down: visible ? listView : null
    }
    ListView {
      id: listView
      Layout.fillWidth: true
      Layout.fillHeight: true
      focus: !searchBar.visible
      clip: true
      boundsBehavior: ListView.StopAtBounds
      boundsMovement: ListView.StopAtBounds
      highlightMoveDuration: 100
      ScrollBar.vertical: ScrollBar {}
      model: textModel
      KeyNavigation.up: searchBar.visible ? searchBar : null
      Keys.onPressed: function(e) {
        if (e.matches(StandardKey.MoveToStartOfDocument)) {
          if (currentIndex !== 0) {
            e.accepted = true;
          }
          currentIndex = 0;
        } else if (e.matches(StandardKey.MoveToEndOfDocument)) {
          if (currentIndex !== count - 1) {
            e.accepted = true;
          }
          currentIndex = count -1;
        } else if (e.key === Qt.Key_PageUp) {
          if (currentIndex !== 0) {
            e.accepted = true;
          }
          currentIndex = Math.max(currentIndex - 10, 0);
        } else if (e.key === Qt.Key_PageDown) {
          if (currentIndex !== count - 1) {
            e.accepted = true;
          }
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
                listView.forceActiveFocus();
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
          text: "Select All"
          shortcut: "Ctrl+A"
          enabled: listView.activeFocus
          onTriggered: textModel.selectAll()
        }
        MenuItem {
          text: "Find"
          shortcut: "Ctrl+F"
          enabled: listView.activeFocus
          onTriggered: searchBar.display(textModel.getSelectionOffset(), textModel.getSelectedText())
        }
      }
    }
  }
}
