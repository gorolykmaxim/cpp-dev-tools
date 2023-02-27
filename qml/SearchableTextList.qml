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
  function ifCurrentItem(field, callback) {
    textList.ifCurrentItem(field, callback);
  }
  function forceActiveFocus() {
    input.forceActiveFocus();
  }
  spacing: 0
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
      text: searchableModel.filter
      placeholderText: root.searchPlaceholderText
      list: textList
      onDisplayTextChanged: searchableModel.filter = displayText
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
      onItemLeftClicked: itemSelected()
    }
  }
}
