import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import cdt
import "." as Cdt

ListView {
  id: root
  property int elide: Text.ElideNone
  signal itemLeftClicked(clickedItemModel: QtObject, event: QtObject);
  signal itemRightClicked(clickedItemModel: QtObject, event: QtObject);
  signal itemSelected();
  function ifCurrentItem(field, fn) {
    if (currentItem) {
      fn(currentItem.itemModel[field]);
    }
  }
  Connections {
    target: model
    function onPreSelectCurrentIndex(index) {
      currentIndex = index;
    }
  }
  onCurrentIndexChanged: {
    if (!model.isUpdating) {
      itemSelected();
    }
  }
  clip: true
  boundsBehavior: ListView.StopAtBounds
  boundsMovement: ListView.StopAtBounds
  highlightMoveDuration: 100
  ScrollBar.vertical: ScrollBar {}
  delegate: Rectangle {
    property var isSelected: ListView.isCurrentItem
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
