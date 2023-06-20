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
  color: Theme.colorBgDark
  GitDiffModel {
    id: diffModel
    onModelChanged: {
      listView.currentIndex = 0;
      listView.positionViewAtIndex(listView.currentIndex, ListView.Center);
    }
  }
  onActiveFocusChanged: {
    if (!activeFocus) {
      diffModel.resetSelection();
    }
  }
  Keys.onEscapePressed: function(e) {
    if (diffModel.resetSelection()) {
      e.accepted = true;
    }
  }
  ListView {
    id: listView
    anchors.fill: parent
    model: diffModel
    focus: true
    clip: true
    boundsBehavior: ListView.StopAtBounds
    boundsMovement: ListView.StopAtBounds
    highlightMoveDuration: 100
    ScrollBar.vertical: ScrollBar {}
    section.property: "header"
    section.labelPositioning: ViewSection.CurrentLabelAtStart
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
    delegate: RowLayout {
      id: listItem
      property bool isSelected: ListView.isCurrentItem
      property int itemIndex: index
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
          if (diffModel.selectedSide === 0) {
            beforeTextLine.rehighlightIfInRange(first, last);
          } else {
            afterTextLine.rehighlightIfInRange(first, last);
          }
        }
      }
      Cdt.TextAreaLine {
        id: beforeTextLine
        Layout.fillHeight: true
        Layout.preferredWidth: listItem.width / 2
        itemIndex: listItem.itemIndex
        itemOffset: 0
        lineNumberMaxWidth: diffModel.maxLineNumberWidthBefore
        text: model.beforeLine
        displayLineNumber: true
        enabled: root.enabled
        lineNumber: model.beforeLineNumber
        formatters: [diffModel.syntaxFormatter, diffModel.selectionFormatter]
        color: listItem.isSelected && listView.activeFocus && diffModel.selectedSide === 0 ? Theme.colorBgMedium : "transparent"
        lineColor: model.isDelete ? "#4df85149" : (model.isAdd ? "#1af85149" : "transparent")
        onInlineSelect: (l, s, e) => diffModel.selectInline(l, s, e)
        onCtrlLeftClick: diffModel.selectLine(listItem.itemIndex)
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
        itemIndex: listItem.itemIndex
        itemOffset: 0
        lineNumberMaxWidth: diffModel.maxLineNumberWidthAfter
        text: model.afterLine
        displayLineNumber: true
        enabled: root.enabled
        lineNumber: model.afterLineNumber
        formatters: [diffModel.syntaxFormatter, diffModel.selectionFormatter]
        color: listItem.isSelected && listView.activeFocus && diffModel.selectedSide === 1 ? Theme.colorBgMedium : "transparent"
        lineColor: model.isAdd ? "#4d3fb950" : (model.isDelete ? "#262ea043" : "transparent")
        onInlineSelect: (l, s, e) => diffModel.selectInline(l, s, e)
        onCtrlLeftClick: diffModel.selectLine(listItem.itemIndex)
        onRightClick: contextMenu.open()
        onPressed: {
          listView.currentIndex = itemIndex;
          listView.forceActiveFocus();
          diffModel.selectedSide = 1;
        }
      }
    }
    Menu {
      id: contextMenu
      MenuItem {
        text: "Copy"
        shortcut: "Ctrl+C"
        enabled: listView.activeFocus
        onTriggered: diffModel.copySelection(listView.currentIndex)
      }
      MenuItem {
        text: "Toggle Select"
        shortcut: "Ctrl+S"
        enabled: listView.activeFocus
        onTriggered: diffModel.selectLine(listView.currentIndex)
      }
      MenuItem {
        text: "Select All"
        shortcut: "Ctrl+A"
        enabled: listView.activeFocus
        onTriggered: diffModel.selectAll()
      }
    }
  }
}