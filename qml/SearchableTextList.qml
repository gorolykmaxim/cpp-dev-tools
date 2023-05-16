import QtQuick
import QtQuick.Layouts
import cdt
import "." as Cdt

FocusScope {
  id: root
  property var searchableModel
  property string searchPlaceholderText: ""
  signal itemSelected(selectedItemModel: QtObject)
  signal itemRightClicked(clickedItemModel: QtObject)
  function ifCurrentItem(field, callback) {
    textList.ifCurrentItem(field, callback);
  }
  function selectCurrentItemIfPresent() {
    if (textList.currentItem) {
      itemSelected(textList.currentItem.itemModel);
    }
  }
  ColumnLayout {
    spacing: 0
    anchors.fill: parent
    Cdt.Pane {
      Layout.fillWidth: true
      color: Theme.colorBgMedium
      padding: Theme.basePadding
      visible: !searchableModel.placeholderText
      focus: true
      Cdt.ListSearch {
        id: input
        width: parent.width
        placeholderText: root.searchPlaceholderText
        list: textList
        listModel: searchableModel
        focus: true
        onEnterPressed: selectCurrentItemIfPresent()
      }
    }
    Cdt.TextList {
      id: textList
      Layout.fillWidth: true
      Layout.fillHeight: true
      model: searchableModel
      elide: Text.ElideLeft
      onItemLeftClicked: selectCurrentItemIfPresent()
      onItemRightClicked: item => root.itemRightClicked(item)
    }
  }
}
