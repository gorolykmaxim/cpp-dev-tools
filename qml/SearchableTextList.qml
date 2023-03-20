import QtQuick
import QtQuick.Layouts
import cdt
import "." as Cdt

FocusScope {
  id: root
  property var searchableModel: null
  property string searchPlaceholderText: ""
  property string placeholderText: ""
  property bool showPlaceholder: false
  signal itemSelected(selectedItemModel: QtObject)
  signal itemRightClicked(clickedItemModel: QtObject)
  signal currentItemChanged()
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
    Cdt.Text {
      Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
      visible: showPlaceholder
      text: placeholderText
    }
    Cdt.Pane {
      Layout.fillWidth: true
      color: Theme.colorBgMedium
      padding: Theme.basePadding
      visible: !showPlaceholder
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
    Cdt.Pane {
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: Theme.colorBgDark
      visible: !showPlaceholder
      Cdt.TextList {
        id: textList
        anchors.fill: parent
        model: searchableModel
        elide: Text.ElideLeft
        onItemLeftClicked: selectCurrentItemIfPresent()
        onItemRightClicked: item => root.itemRightClicked(item)
        onCurrentIndexChanged: root.currentItemChanged()
      }
    }
  }
}
