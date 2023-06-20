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
  color: Theme.colorBgDark
  signal itemLeftClicked(clickedItemModel: QtObject, event: QtObject);
  signal itemRightClicked(clickedItemModel: QtObject, event: QtObject);
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
    textColor: list.model.placeholderColor
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
      property var isSelected: ListView.isCurrentItem && (highlightCurrentItemWithoutFocus || list.activeFocus)
      property var itemModel: model
      width: list.width
      height: row.height
      color: (ListView.isCurrentItem || mouseArea.containsMouse) && !(mouseArea.pressedButtons & Qt.LeftButton) ? Theme.colorBgMedium : "transparent"
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
            highlight: isSelected
            textColor: itemModel.titleColor || Theme.colorText
          }
          Cdt.Text {
            width: text ? parent.width : null
            elide: root.elide
            text: itemModel.subTitle || ""
            color: Theme.colorSubText
          }
        }
        Cdt.Text {
          Layout.alignment: Qt.AlignRight
          Layout.margins: Theme.basePadding
          text: itemModel.rightText || ""
          color: Theme.colorSubText
        }
      }
      MouseArea {
        id: mouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true
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
