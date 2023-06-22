import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import "Common.js" as Common
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
    textModel.rehighlightLines(line, line);
  }
  color: Theme.colorBgDark
  BigTextAreaModel {
    id: textModel
    onGoToLine: function(line) {
      listView.currentIndex = line;
      listView.positionViewAtIndex(listView.currentIndex, ListView.Center);
    }
  }
  Keys.onEscapePressed: function(e) {
    e.accepted = textModel.resetSelection() || searchBar.close();
  }
  ColumnLayout {
    anchors.fill: parent
    spacing: 0
    Cdt.TextAreaSearchBar {
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
      Keys.onPressed: e => Common.handleListViewNavigation(e, listView)
      delegate: Cdt.TextAreaLine {
        required property var model
        required property int index
        id: textAreaLine
        itemIndex: index
        itemOffset: model.offset
        lineNumberMaxWidth: textModel.lineNumberMaxWidth
        text: model.text
        formatters: [...root.formatters, searchBar.formatter, textModel.selectionFormatter]
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
            callback: () => searchBar.displayAndGoToResult(textModel.getSelectionOffset(), textModel.getSelectedText()),
          },
          ...additionalMenuItems
        ]
      }
    }
  }
}
