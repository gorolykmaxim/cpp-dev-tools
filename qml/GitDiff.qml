import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import cdt
import "." as Cdt

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
        if (e.key === Qt.Key_Left && !diffModel.isBeforeSelected) {
          diffModel.isBeforeSelected = true;
          e.accepted = true;
        } else if (e.key === Qt.Key_Right && diffModel.isBeforeSelected) {
          diffModel.isBeforeSelected = false;
          e.accepted = true;
        } else {
          e.accepted = false;
        }
      }
      Cdt.TextAreaLine {
        Layout.fillHeight: true
        Layout.preferredWidth: listItem.width / 2
        itemIndex: listItem.itemIndex
        itemOffset: 0
        lineNumberMaxWidth: diffModel.maxLineNumberWidthBefore
        text: model.beforeLine
        displayLineNumber: true
        enabled: root.enabled
        lineNumber: model.beforeLineNumber
        formatters: [diffModel.formatter]
        color: listItem.isSelected && listView.activeFocus && diffModel.isBeforeSelected ? Theme.colorBgMedium : "transparent"
        lineColor: model.isDelete ? "#4df85149" : (model.isAdd ? "#1af85149" : "transparent")
        onPressed: {
          listView.currentIndex = itemIndex;
          listView.forceActiveFocus();
          diffModel.isBeforeSelected = true;
        }
      }
      Cdt.TextAreaLine {
        Layout.fillHeight: true
        Layout.preferredWidth: listItem.width / 2
        itemIndex: listItem.itemIndex
        itemOffset: 0
        lineNumberMaxWidth: diffModel.maxLineNumberWidthAfter
        text: model.afterLine
        displayLineNumber: true
        enabled: root.enabled
        lineNumber: model.afterLineNumber
        formatters: [diffModel.formatter]
        color: listItem.isSelected && listView.activeFocus && !diffModel.isBeforeSelected ? Theme.colorBgMedium : "transparent"
        lineColor: model.isAdd ? "#4d3fb950" : (model.isDelete ? "#262ea043" : "transparent")
        onPressed: {
          listView.currentIndex = itemIndex;
          listView.forceActiveFocus();
          diffModel.isBeforeSelected = false;
        }
      }
    }
  }
}
