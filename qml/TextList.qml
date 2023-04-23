import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import cdt
import "." as Cdt

ListView {
  id: root
  property int elide: Text.ElideNone
  property bool highlightCurrentItemWithoutFocus: true
  signal itemLeftClicked(clickedItemModel: QtObject, event: QtObject);
  signal itemRightClicked(clickedItemModel: QtObject, event: QtObject);
  signal itemSelected();
  function ifCurrentItem(field, fn) {
    if (currentItem) {
      fn(currentItem.itemModel[field]);
    }
  }
  function pageUp() {
    const result = currentIndex - 10;
    currentIndex = result < 0 ? 0 : result;
  }
  function pageDown() {
    const result = currentIndex + 10;
    currentIndex = result >= count ? count - 1 : result;
  }
  Connections {
    target: model
    function onPreSelectCurrentIndex(index) {
      currentIndex = index;
      itemSelected();
    }
  }
  onCurrentIndexChanged: {
    if (!model.isUpdating) {
      itemSelected();
    }
  }
  Keys.onPressed: e => {
    switch (e.key) {
      case Qt.Key_PageUp:
        pageUp();
        break;
      case Qt.Key_PageDown:
        pageDown();
        break;
    }
  }
  clip: true
  boundsBehavior: ListView.StopAtBounds
  boundsMovement: ListView.StopAtBounds
  highlightMoveDuration: 100
  ScrollBar.vertical: ScrollBar {}
  delegate: Rectangle {
    property var isSelected: ListView.isCurrentItem && (highlightCurrentItemWithoutFocus || root.activeFocus)
    property var itemModel: model
    width: root.width
    height: row.height
    color: (isSelected || mouseArea.containsMouse) && !(mouseArea.pressedButtons & Qt.LeftButton) ? Theme.colorBgMedium : "transparent"
    RowLayout {
      id: row
      width: parent.width
      spacing: 0
      Cdt.Icon {
        icon: itemModel.icon || ""
        color: itemModel.iconColor || Theme.colorText
        visible: itemModel.icon || false
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
        root.currentIndex = index;
        if (e.button === Qt.LeftButton) {
          root.itemLeftClicked(itemModel, e);
        } else {
          root.itemRightClicked(itemModel, e);
        }
      }
    }
  }
}
