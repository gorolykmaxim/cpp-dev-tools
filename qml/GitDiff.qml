import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import cdt
import "." as Cdt
import "Common.js" as Common

Cdt.Pane {
  id: root
  property alias file: diffModel.file
  property alias rawDiff: diffModel.rawDiff
  property alias chunkCount: diffModel.chunkCount
  property alias currentChunk: diffModel.currentChunk
  property var additionalMenuItems: []
  color: Theme.colorBgDark
  GitDiffModel {
    id: diffModel
    onGoToLine: function(line) {
      listView.currentIndex = line;
      listView.positionViewAtIndex(listView.currentIndex, ListView.Center);
    }
    onModelChanged: {
      if (searchBar.visible) {
        search(searchBar.searchTerm);
      }
    }
  }
  onActiveFocusChanged: {
    if (!activeFocus) {
      diffModel.resetSelection();
    }
  }
  Keys.onEscapePressed: function(e) {
    e.accepted = diffModel.resetSelection() || searchBar.close();
  }
  ColumnLayout {
    anchors.fill: parent
    spacing: 0
    Cdt.TextSearchBar {
      id: searchBar
      Layout.fillWidth: true
      readOnly: true
      searchResultCount: diffModel.searchResultsCount
      noSearchResults: diffModel.searchResultsEmpty
      KeyNavigation.down: visible ? listView : null
      function displayAndGoToResult(line, text) {
        const side = diffModel.selectedSide;
        display(text);
        diffModel.goToSearchResultInLineAndSide(line, side);
      }
      onSearch: diffModel.search(searchTerm)
      onGoToSearchResult: next => diffModel.goToSearchResult(next)
    }
    ListView {
      id: listView
      Layout.fillWidth: true
      Layout.fillHeight: true
      model: diffModel
      focus: !searchBar.visible
      clip: true
      enabled: root.enabled
      boundsBehavior: ListView.StopAtBounds
      boundsMovement: ListView.StopAtBounds
      highlightMoveDuration: 100
      ScrollBar.vertical: ScrollBar {}
      KeyNavigation.up: searchBar.visible ? searchBar : null
      section.property: "header"
      section.labelPositioning: ViewSection.InlineLabels | ViewSection.CurrentLabelAtStart
      section.delegate: Cdt.Pane {
        required property string section
        width: listView.width
        color: Theme.colorBgBlack
        padding: Theme.basePadding
        Cdt.Text {
          text: section
          color: Theme.colorSubText
        }
      }
      onCurrentIndexChanged: diffModel.selectCurrentChunk(currentIndex)
      delegate: diffModel.isSideBySideView ? sideBySideLine : unifiedLine
      Component {
        id: sideBySideLine
        RowLayout {
          id: listItem
          required property int index
          required property string beforeLine
          required property string afterLine
          required property int beforeLineNumber
          required property int afterLineNumber
          required property bool isAdd
          required property bool isDelete
          property bool isSelected: ListView.isCurrentItem
          width: listView.width
          spacing: 0
          Keys.onPressed: function(e) {
            if (e.key === Qt.Key_Left && diffModel.selectedSide > 0) {
              diffModel.selectedSide--;
              e.accepted = true;
            } else if (e.key === Qt.Key_Right && diffModel.selectedSide < 1) {
              diffModel.selectedSide++;
              e.accepted = true;
            }
            Common.handleListViewNavigation(e, listView);
          }
          Connections {
            target: diffModel
            function onRehighlightLines(first, last) {
              beforeTextLine.rehighlightIfInRange(first, last);
              afterTextLine.rehighlightIfInRange(first, last);
            }
          }
          Cdt.TextAreaLine {
            id: beforeTextLine
            Layout.fillHeight: true
            Layout.preferredWidth: listItem.width / 2
            clip: true
            itemIndex: index
            itemOffset: 0
            lineNumberMaxWidth: diffModel.maxLineNumberWidthBefore
            text: beforeLine
            displayLineNumber: true
            enabled: root.enabled
            lineNumber: beforeLineNumber
            formatters: [diffModel.syntaxFormatter, diffModel.beforeSearchFormatter, diffModel.beforeSelectionFormatter]
            color: listItem.isSelected && listView.activeFocus && diffModel.selectedSide === 0 ? Theme.colorBgMedium : "transparent"
            lineColor: isDelete ? "#4df85149" : (isAdd ? "#1af85149" : "transparent")
            onInlineSelect: (l, s, e) => diffModel.selectInline(l, s, e)
            onCtrlLeftClick: diffModel.selectLine(index)
            onRightClick: contextMenu.open()
            onPressed: {
              listView.currentIndex = itemIndex;
              listView.forceActiveFocus();
              diffModel.selectedSide = 0;
            }
          }
          Cdt.TextAreaLine {
            id: afterTextLine
            Layout.fillHeight: true
            Layout.preferredWidth: listItem.width / 2
            clip: true
            itemIndex: index
            itemOffset: 0
            lineNumberMaxWidth: diffModel.maxLineNumberWidthAfter
            text: afterLine
            displayLineNumber: true
            enabled: root.enabled
            lineNumber: afterLineNumber
            formatters: [diffModel.syntaxFormatter, diffModel.afterSearchFormatter, diffModel.afterSelectionFormatter]
            color: listItem.isSelected && listView.activeFocus && diffModel.selectedSide === 1 ? Theme.colorBgMedium : "transparent"
            lineColor: isAdd ? "#4d3fb950" : (isDelete ? "#262ea043" : "transparent")
            onInlineSelect: (l, s, e) => diffModel.selectInline(l, s, e)
            onCtrlLeftClick: diffModel.selectLine(index)
            onRightClick: contextMenu.open()
            onPressed: {
              listView.currentIndex = itemIndex;
              listView.forceActiveFocus();
              diffModel.selectedSide = 1;
            }
          }
        }
      }
      Component {
        id: unifiedLine
        Cdt.TextAreaLine {
          id: textLine
          required property int index
          required property string beforeLine
          required property int beforeLineNumber
          required property bool isDelete
          required property bool isAdd
          property bool isSelected: ListView.isCurrentItem
          width: listView.width
          clip: true
          itemIndex: index
          itemOffset: 0
          lineNumberMaxWidth: Math.max(diffModel.maxLineNumberWidthBefore, diffModel.maxLineNumberWidthAfter)
          text: beforeLine
          displayLineNumber: true
          enabled: root.enabled
          lineNumber: beforeLineNumber
          formatters: [diffModel.syntaxFormatter, diffModel.beforeSearchFormatter, diffModel.beforeSelectionFormatter]
          color: isSelected && listView.activeFocus ? Theme.colorBgMedium : "transparent"
          lineColor: isDelete ? "#4df85149" : (isAdd ? "#4d3fb950" : "transparent")
          onInlineSelect: (l, s, e) => diffModel.selectInline(l, s, e)
          onCtrlLeftClick: diffModel.selectLine(itemIndex)
          onRightClick: contextMenu.open()
          onPressed: {
            listView.currentIndex = itemIndex;
            listView.forceActiveFocus();
            diffModel.selectedSide = 0;
          }
          Keys.onPressed: e => Common.handleListViewNavigation(e, listView)
          Connections {
            target: diffModel
            function onRehighlightLines(first, last) {
              textLine.rehighlightIfInRange(first, last);
            }
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
            callback: () => diffModel.copySelection(listView.currentIndex),
          },
          {
            text: "Toggle Select",
            shortcut: "Ctrl+S",
            callback: () => diffModel.selectLine(listView.currentIndex),
          },
          {
            text: "Select All",
            shortcut: "Ctrl+A",
            callback: () => diffModel.selectAll(),
          },
          {
            text: "Find",
            shortcut: "Ctrl+F",
            callback: () => searchBar.displayAndGoToResult(listView.currentIndex, diffModel.getSelectedText()),
          },
          {
            separator: true,
          },
          {
            text: "Open Chunk In Editor",
            shortcut: "Ctrl+O",
            callback: () => diffModel.openFileInEditor(listView.currentIndex),
          },
          {
            text: "Toggle Unified Diff",
            shortcut: "Alt+U",
            callback: () => diffModel.isSideBySideView = !diffModel.isSideBySideView,
          },
          ...additionalMenuItems
        ]
      }
    }
  }
}
