import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
  Component.onCompleted: {
    root.model.onModelReset.connect(() => root.currentIndex = 0);
  }
  id: root
  signal itemClicked(clickedItemModel: QtObject, event: QtObject);
  clip: true
  boundsBehavior: ListView.StopAtBounds
  delegate: Rectangle {
    property var isSelected: ListView.isCurrentItem
    property var itemModel: model
    width: root.width
    height: row.height
    color: isSelected ? colorBgMedium : "transparent"
    RowLayout {
      id: row
      width: parent.width
      Column {
        Layout.margins: basePadding
        TextWidget {
          text: itemModel.title || ""
          highlight: isSelected
        }
        TextWidget {
          text: itemModel.subTitle || ""
          color: colorSubText
        }
      }
      TextWidget {
        Layout.alignment: Qt.AlignRight
        Layout.margins: basePadding
        text: itemModel.rightText || ""
        color: colorSubText
      }
    }
    MouseArea {
      anchors.fill: parent
      acceptedButtons: Qt.LeftButton | Qt.RightButton
      onClicked: e => {
        root.currentIndex = index;
        root.itemClicked(itemModel, e);
      }
    }
  }
}
