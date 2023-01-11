import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
  Component.onCompleted: {
    root.model.onModelReset.connect(() => {
      root.currentIndex = root.onUpdateSelectLast ? root.count - 1: 0;
    });
  }
  id: root
  property var onUpdateSelectLast: false
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
      spacing: basePadding
      IconWidget {
        icon: itemModel.icon || ""
        color: itemModel.iconColor || colorText
        Layout.leftMargin: basePadding
        Layout.topMargin: basePadding
        Layout.bottomMargin: basePadding
      }
      Column {
        Layout.topMargin: basePadding
        Layout.bottomMargin: basePadding
        Layout.fillWidth: true
        TextWidget {
          text: itemModel.title || ""
          highlight: isSelected
          textColor: itemModel.titleColor || colorText
        }
        TextWidget {
          text: itemModel.subTitle || ""
          color: colorSubText
        }
      }
      TextWidget {
        Layout.alignment: Qt.AlignRight
        Layout.rightMargin: basePadding
        Layout.topMargin: basePadding
        Layout.bottomMargin: basePadding
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
