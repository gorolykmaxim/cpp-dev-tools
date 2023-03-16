import QtQuick
import QtQuick.Layouts
import cdt
import "." as Cdt

ColumnLayout {
  id: root
  property var searchableModel: null
  property string searchPlaceholderText: ""
  property string placeholderText: ""
  property bool showPlaceholder: false
  property alias hasFocus: input.activeFocus
  property var navigationUp: null
  property var navigationDown: null
  property var navigationLeft: null
  property var navigationRight: null
  signal itemSelected()
  signal itemRightClicked()
  signal currentItemChanged()
  function ifCurrentItem(field, callback) {
    textList.ifCurrentItem(field, callback);
  }
  onActiveFocusChanged: {
    if (activeFocus) {
      input.forceActiveFocus();
    }
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
      placeholderText: root.searchPlaceholderText
      list: textList
      listModel: searchableModel
      onEnterPressed: itemSelected()
      KeyNavigation.up: navigationUp
      KeyNavigation.down: navigationDown
      KeyNavigation.left: navigationLeft
      KeyNavigation.right: navigationRight
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
      onItemRightClicked: root.itemRightClicked()
      onCurrentIndexChanged: root.currentItemChanged()
    }
  }
}
