import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt

Cdt.Pane {
  id: root
  property string text: ""
  property string currentLineColor: Theme.colorBgMedium
  property bool highlightCurrentLineWithoutFocus: false
  property bool monoFont: true
  property alias cursorFollowEnd: textModel.cursorFollowEnd
  property alias cursorPosition: textModel.cursorPosition
  property alias currentLine: listView.currentIndex
  property bool displayLineNumbers: false
  property list<QtObject> formatters: []
  property var additionalMenuItems: []
  signal preHighlight()
  onTextChanged: {
    searchBar.text = text;
    preHighlight();
    textModel.text = text;
  }
  onActiveFocusChanged: {
    if (!activeFocus) {
      textModel.resetSelection();
    }
  }
  function goToLine(line) {
    textModel.goToLine(line);
  }
  function rehighlightLine(line) {
    textModel.rehighlightLine(line);
  }
  color: Theme.colorBgDark
  BigTextAreaModel {
    id: textModel
    formatters: root.formatters.concat([searchBar.formatter])
    onGoToLine: function(line) {
      listView.currentIndex = line;
      listView.positionViewAtIndex(listView.currentIndex, ListView.Center);
    }
  }
  Keys.onEscapePressed: function(e) {
    if (textModel.resetSelection() || searchBar.close()) {
      e.accepted = true;
    }
  }
  ColumnLayout {
    anchors.fill: parent
    spacing: 0
    Cdt.TextSearchBar {
      id: searchBar
      Layout.fillWidth: true
      readOnly: true
      onSearchResultsChanged: textModel.rehighlight()
      onSelectResult: (o, l) => textModel.selectSearchResult(o, l)
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
      delegate: Cdt.TextAreaLine {
        required property var model
        required property int index
        id: textAreaLine
        itemIndex: index
        itemOffset: model.offset
        lineNumberMaxWidth: textModel.lineNumberMaxWidth
        text: model.text
        formatters: textModel.formatters
        displayLineNumber: root.displayLineNumbers
        enabled: root.enabled
        width: listView.width
        color: ListView.isCurrentItem && (root.highlightCurrentLineWithoutFocus || listView.activeFocus) ? root.currentLineColor : "transparent"
        onInlineSelect: (l, s, e) => textModel.selectInline(l, s, e)
        onPressed: {
          listView.currentIndex = index;
          listView.forceActiveFocus();
        }
        onRightClick: contextMenu.open()
        onCtrlLeftClick: textModel.selectLine(index)
        Connections {
          target: textModel
          function onRehighlightLines(first, last) {
            textAreaLine.rehighlightIfInRange(first, last);
          }
        }
      }
      Cdt.DynamicContextMenu {
        id: contextMenu
        parentView: listView
        itemsList: [
          {
            text: "Copy",
            shortcut: "Ctrl+C",
            callback: () => textModel.copySelection(listView.currentIndex),
          },
          {
            text: "Toggle Select",
            shortcut: "Ctrl+S",
            callback: () => textModel.selectLine(listView.currentIndex),
          },
          {
            text: "Select All",
            shortcut: "Ctrl+A",
            callback: () => textModel.selectAll(),
          },
          {
            text: "Find",
            shortcut: "Ctrl+F",
            callback: () => searchBar.display(textModel.getSelectionOffset(), textModel.getSelectedText()),
          },
          ...additionalMenuItems
        ]
      }
    }
  }
}
