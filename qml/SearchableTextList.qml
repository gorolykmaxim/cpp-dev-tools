import QtQuick.Layouts
import cdt
import "." as Cdt

ColumnLayout {
  id: root
  property var searchableModel: null
  property string searchPlaceholderText: ""
  property string placeholderText: ""
  property bool showPlaceholder: false
  signal itemSelected()
  signal currentItemChanged()
  function ifCurrentItem(field, callback) {
    textList.ifCurrentItem(field, callback);
  }
  function forceActiveFocus() {
    input.forceActiveFocus();
  }
  spacing: 0
  onActiveFocusChanged: {
    if (activeFocus) {
      forceActiveFocus();
    }
  }
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
    Cdt.ListSearch {
      id: input
      width: parent.width
      placeholderText: root.searchPlaceholderText
      list: textList
      listModel: searchableModel
      onEnterPressed: itemSelected()
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
      onItemLeftClicked: itemSelected()
      onCurrentIndexChanged: root.currentItemChanged()
    }
  }
}
