import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
  function selectCurrentItem() {
    let toSelect = 0;
    for (let i = 0; i < root.model.rowCount(); i++) {
      if (root.model.GetFieldByRoleName(i, "isSelected")) {
        toSelect = i;
      }
    }
    root.currentIndex = toSelect;
  }
  onModelChanged: {
    if (!root.model) {
      return;
    }
    selectCurrentItem();
    root.model.onModelReset.connect(selectCurrentItem);
  }
  id: root
  property var elide: Text.ElideNone
  signal itemLeftClicked(clickedItemModel: QtObject, event: QtObject);
  signal itemRightClicked(clickedItemModel: QtObject, event: QtObject);
  clip: true
  boundsBehavior: ListView.StopAtBounds
  delegate: Rectangle {
    property var isSelected: ListView.isCurrentItem
    property var itemModel: model
    width: root.width
    height: row.height
    color: (isSelected || mouseArea.containsMouse) && !mouseArea.pressed ? colorBgMedium : "transparent"
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
          width: text ? parent.width : null
          text: itemModel.title || ""
          elide: root.elide
          highlight: isSelected
          textColor: itemModel.titleColor || colorText
        }
        TextWidget {
          width: text ? parent.width : null
          elide: root.elide
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
      id: mouseArea
      anchors.fill: parent
      acceptedButtons: Qt.LeftButton | Qt.RightButton
      hoverEnabled: true
      onClicked: e => {
        root.currentIndex = index;
        if (e.button == Qt.LeftButton) {
          root.itemLeftClicked(itemModel, e);
        } else {
          root.itemRightClicked(itemModel, e);
        }
      }
    }
  }
}
