import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import cdt
import "." as Cdt

Cdt.Pane {
  id: root
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
        color: listItem.isSelected && listView.activeFocus ? Theme.colorBgMedium : "transparent"
        lineColor: model.isDelete ? "#4df85149" : (model.isAdd ? "#1af85149" : "transparent")
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
        color: listItem.isSelected && listView.activeFocus ? Theme.colorBgMedium : "transparent"
        lineColor: model.isAdd ? "#4d3fb950" : (model.isDelete ? "#262ea043" : "transparent")
      }
    }
  }
}
