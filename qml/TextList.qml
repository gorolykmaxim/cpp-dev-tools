import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import cdt
import "." as Cdt

Cdt.Pane {
  id: root
  property int elide: Text.ElideNone
  property bool highlightCurrentItemWithoutFocus: true
  property alias model: list.model
  property alias placeholderText: placeholder.text
  property alias placeholderColor: placeholder.textColor
  property alias currentItem: list.currentItem
  property bool showPlaceholder: list.count === 0
  color: Theme.colorBgDark
  signal itemLeftClicked(clickedItemModel: QtObject, event: QtObject);
  signal itemRightClicked(clickedItemModel: QtObject, event: QtObject);
  function ifCurrentItem(field, fn) {
    if (list.currentItem) {
      fn(list.currentItem.itemModel[field]);
    }
  }
  function incrementCurrentIndex() {
    list.incrementCurrentIndex();
  }
  function decrementCurrentIndex() {
    list.decrementCurrentIndex();
  }
  function pageUp() {
    const result = list.currentIndex - 10;
    list.currentIndex = result < 0 ? 0 : result;
  }
  function pageDown() {
    const result = list.currentIndex + 10;
    list.currentIndex = result >= list.count ? list.count - 1 : result;
  }
  Cdt.PlaceholderText {
    id: placeholder
    anchors.fill: parent
    visible: root.showPlaceholder
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
    Keys.onPressed: e => {
      switch (e.key) {
        case Qt.Key_PageUp:
          root.pageUp();
          break;
        case Qt.Key_PageDown:
          root.pageDown();
          break;
      }
    }
    anchors.fill: parent
    visible: !root.showPlaceholder
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
