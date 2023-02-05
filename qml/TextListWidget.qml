import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
  onModelChanged: {
    if (!root.model) {
      return;
    }
    root.model.onModelReset.connect(() => {
      let toSelect = 0;
      for (let i = 0; i < root.model.rowCount(); i++) {
        if (root.model.GetFieldByRoleName(i, "isSelected")) {
          toSelect = i;
        }
      }
      root.currentIndex = toSelect;
    });
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
      spacing: 0
      IconWidget {
        icon: itemModel.icon || ""
        color: itemModel.iconColor || colorText
        visible: itemModel.icon || false
        Layout.leftMargin: basePadding
        Layout.topMargin: basePadding
        Layout.bottomMargin: basePadding
      }
      Column {
        Layout.topMargin: basePadding
        Layout.bottomMargin: basePadding
        Layout.leftMargin: basePadding
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
