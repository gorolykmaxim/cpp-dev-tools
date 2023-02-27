import QtQuick.Layouts
import cdt
import "." as Cdt

ColumnLayout {
  id: root
  property var searchableModel: null
  property string placeholderText: ""
  signal itemSelected()
  function ifCurrentItem(field, callback) {
    textList.ifCurrentItem(field, callback);
  }
  function forceActiveFocus() {
    input.forceActiveFocus();
  }
  spacing: 0
  Cdt.Pane {
    Layout.fillWidth: true
    color: Theme.colorBgMedium
    padding: Theme.basePadding
    Cdt.ListSearch {
      id: input
      width: parent.width
      text: searchableModel.filter
      placeholderText: root.placeholderText
      list: textList
      onDisplayTextChanged: searchableModel.filter = displayText
      onEnterPressed: itemSelected()
    }
  }
  Cdt.Pane {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Theme.colorBgDark
    Cdt.TextList {
      id: textList
      anchors.fill: parent
      model: searchableModel
      onItemLeftClicked: itemSelected()
    }
  }
}
