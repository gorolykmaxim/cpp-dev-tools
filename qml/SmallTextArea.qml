import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import Qt.labs.platform
import cdt
import "." as Cdt
import "Common.js" as Common

FocusScope {
  id: root
  property alias text: textArea.text
  property alias placeholderText: textArea.placeholderText
  property alias effectiveCursorPosition: textArea.cursorPosition
  property bool ignoreTextChange: false
  property QtObject formatter: DummyFormatter {}
  signal ctrlEnterPressed()
  signal preHighlight()
  function goToPage(event, down) {
    let pos = textArea.cursorPosition;
    let delta = down ? 1 : -1;
    let lines = 0;
    for (; (down ? pos < textArea.length : pos > 0) && lines < 20; pos += delta) {
      if (textArea.text[pos] === '\n') {
        lines++;
      }
    }
    event.accepted = textArea.cursorPosition !== pos;
    textArea.cursorPosition = pos;
  }
  Keys.onEscapePressed: function(e) {
    if (textArea.selectedText) {
      textArea.deselect();
      e.accepted = true;
    } else if (searchBar.close()) {
      e.accepted = true;
    }
  }
  SmallTextAreaHighlighter {
    id: highlighter
    formatters: [root.formatter, searchBar.formatter]
    document: textArea.textDocument
  }
  ColumnLayout {
    spacing: 0
    anchors.fill: parent
    Cdt.TextSearchBar {
      id: searchBar
      Layout.fillWidth: true
      onSelectResult: function(o, l) {
        ignoreTextChange = true;
        textArea.select(o, o + l);
        ignoreTextChange = false;
      }
      onReplaceResults: function(offsets, lengths, text) {
        for (let i = 0; i < offsets.length; i++) {
          const offset = offsets[i];
          const length = lengths[i];
          textArea.remove(offset, offset + length);
          textArea.insert(offset, text);
        }
      }
      onSearchResultsChanged: {
        ignoreTextChange = true;
        highlighter.rehighlight();
        ignoreTextChange = false;
      }
      KeyNavigation.down: visible ? textArea : null
    }
    Cdt.Pane {
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: Theme.colorBgDark
      focus: !searchBar.visible
      Controls.ScrollView {
        anchors.fill: parent
        focus: true
        Keys.forwardTo: [root]
        Controls.TextArea {
          id: textArea
          selectByMouse: true
          enabled: root.enabled
          selectionColor: Theme.colorHighlight
          color: enabled ? Theme.colorText : Theme.colorSubText
          leftPadding: Theme.basePadding
          rightPadding: Theme.basePadding
          topPadding: Theme.basePadding
          bottomPadding: Theme.basePadding
          textFormat: TextEdit.PlainText
          placeholderTextColor: Theme.colorSubText
          font.family: monoFontFamily
          font.pointSize: monoFontSize
          renderType: Text.NativeRendering
          focus: true
          wrapMode: Controls.TextArea.WordWrap
          background: Rectangle {
            color: "transparent"
          }
          onTextChanged: {
            // When TextSearchBar is open and has a search term entered,
            // if you type anything in the TextArea:
            // - text gets changed
            // - search() of TextSearchBar gets called
            // - selectResult or searchResultsChanged get emitted
            // - either TextArea.select() or SmallTextAreaHighlighter.rehighlight() gets called
            // - internal state of TextArea gets modified and text gets "changed"
            // This leads to a binding loop on "text".
            // The code below sets text of TextSearchBar while avoiding the aforementioned loop.
            if (!ignoreTextChange) {
              searchBar.text = text;
              preHighlight();
              ignoreTextChange = true;
              highlighter.rehighlight();
              ignoreTextChange = false;
            }
          }
          Keys.onPressed: function(e) {
            if ((e.key === Qt.Key_Enter || e.key === Qt.Key_Return) && (e.modifiers & Qt.ControlModifier)) {
              root.ctrlEnterPressed();
              e.accepted = true;
            } else if (e.key === Qt.Key_PageUp) {
              goToPage(e, false);
            } else if (e.key === Qt.Key_PageDown) {
              goToPage(e, true);
            }
            e.accepted = false;
          }
          KeyNavigation.up: searchBar.visible ? searchBar : null
          onPressed: e => Common.handleRightClick(textArea, contextMenu, e)
          Menu {
            id: contextMenu
            MenuItem {
              text: "Cut"
              shortcut: "Ctrl+X"
              enabled: textArea.activeFocus
              onTriggered: textArea.cut()
            }
            MenuItem {
              text: "Copy"
              shortcut: "Ctrl+C"
              enabled: textArea.activeFocus
              onTriggered: textArea.copy()
            }
            MenuItem {
              text: "Paste"
              shortcut: "Ctrl+V"
              enabled: textArea.activeFocus
              onTriggered: textArea.paste()
            }
            MenuSeparator {}
            MenuItem {
              text: "Find"
              enabled: textArea.activeFocus
              shortcut: "Ctrl+F"
              onTriggered: searchBar.display(textArea.selectionStart, textArea.selectedText)
            }
            MenuSeparator {}
            MenuItem {
              text: "Select All"
              enabled: textArea.activeFocus
              shortcut: "Ctrl+A"
              onTriggered: textArea.selectAll()
            }
            MenuItem {
              text: "Select Word"
              enabled: textArea.activeFocus
              shortcut: "Ctrl+D"
              onTriggered: textArea.selectWord()
            }
          }
        }
      }
    }
  }
}
