import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import cdt
import "." as Cdt
import "Common.js" as Common

Cdt.Pane {
  id: root
  property int elide: Text.ElideNone
  property bool highlightCurrentItemWithoutFocus: true
  property alias model: list.model
  property alias currentItem: list.currentItem
  signal itemLeftClicked(clickedItemModel: QtObject, event: QtObject);
  signal itemRightClicked(clickedItemModel: QtObject, event: QtObject);
  padding: Theme.basePadding
  function ifCurrentItem(field, fn) {
    if (list.currentItem) {
      fn(list.currentItem.itemModel[field]);
    }
  }
  function incrementCurrentIndex() {
    const old = list.currentIndex;
    list.incrementCurrentIndex();
    return old !== list.currentIndex;
  }
  function decrementCurrentIndex() {
    const old = list.currentIndex;
    list.decrementCurrentIndex();
    return old !== list.currentIndex;
  }
  function pageUp() {
    return Common.pageUpList(list);
  }
  function pageDown() {
    return Common.pageDownList(list);
  }
  Cdt.PlaceholderText {
    id: placeholder
    anchors.fill: parent
    visible: list.model.placeholderText
    text: list.model.placeholderText
    color: list.model.placeholderColor
  }
  ListView {
    id: list
    property bool currentIndexPreSelected: false
    Connections {
      target: list.model
      function onPreSelectCurrentIndex(index) {
        list.currentIndex = index;
        list.currentIndexPreSelected = true;
      }
    }
    onCurrentIndexChanged: {
      if (!model.isUpdating && list.currentIndexPreSelected) {
        model.selectItemByIndex(list.currentIndex);
      }
    }
    Keys.onPressed: e => Common.handleListViewNavigation(e, list)
    anchors.fill: parent
    visible: !list.model.placeholderText
    focus: true
    clip: true
    boundsBehavior: ListView.StopAtBounds
    boundsMovement: ListView.StopAtBounds
    highlightMoveDuration: 100
    ScrollBar.vertical: ScrollBar {}
    delegate: Rectangle {
      property bool isCurrentItem: ListView.isCurrentItem
      property bool isHighlighted: highlightCurrentItemWithoutFocus || list.activeFocus
      property var itemModel: model
      width: list.width
      height: row.height
      radius: Theme.baseRadius
      color: isCurrentItem && !(mouseArea.pressedButtons & Qt.LeftButton) ? (isHighlighted ? Theme.colorPrimary : Theme.colorBorder) : "transparent"
      RowLayout {
        id: row
        width: parent.width
        spacing: 0
        Cdt.Icon {
          icon: itemModel.icon || ""
          color: itemModel.iconColor || Theme.colorText
          visible: itemModel.icon || false
          iconSize: itemModel.subTitle ? 24 : 16
          Layout.leftMargin: Theme.basePadding
          Layout.topMargin: Theme.basePadding
          Layout.bottomMargin: Theme.basePadding
        }
        Column {
          Layout.topMargin: Theme.basePadding
          Layout.bottomMargin: Theme.basePadding
          Layout.leftMargin: Theme.basePadding
          Layout.fillWidth: true
          Cdt.Text {
            width: text ? parent.width : null
            text: itemModel.title || ""
            elide: root.elide
            textColor: isCurrentItem && isHighlighted ? Theme.colorText : (itemModel.titleColor || Theme.colorText)
          }
          Cdt.Text {
            width: text ? parent.width : null
            elide: root.elide
            text: itemModel.subTitle || ""
            color: isCurrentItem && isHighlighted ? Theme.colorText : Theme.colorPlaceholder
          }
        }
        Cdt.Text {
          Layout.alignment: Qt.AlignRight
          Layout.margins: Theme.basePadding
          text: itemModel.rightText || ""
          color: isCurrentItem && isHighlighted ? Theme.colorText : Theme.colorPlaceholder
        }
      }
      MouseArea {
        id: mouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: e => {
          root.forceActiveFocus();
          list.currentIndex = index;
          if (e.button === Qt.LeftButton) {
            root.itemLeftClicked(itemModel, e);
          } else {
            root.itemRightClicked(itemModel, e);
          }
        }
      }
    }
  }
}
