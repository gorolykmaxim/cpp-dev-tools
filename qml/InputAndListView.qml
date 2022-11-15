import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
  Component.onCompleted: {
    list.model.onModelReset.connect(() => list.currentIndex = 0);
  }
  anchors.fill: parent
  spacing: 0
  PaneWidget {
    Layout.fillWidth: true
    color: colorBgMedium
    focus: true
    padding: basePadding
    RowLayout {
      anchors.fill: parent
      TextFieldWidget {
        id: input
        text: dataViewInputValue || ""
        focus: true
        Layout.fillWidth: true
        KeyNavigation.right: button
        onDisplayTextChanged: core.OnUserAction("viewSlot",
                                                "actionViewInputValueChanged",
                                                [displayText])
        Keys.onReturnPressed: core.OnUserAction("viewSlot",
                                                "actionViewItemChosen",
                                                [list.currentIndex])
        Keys.onEnterPressed: core.OnUserAction("viewSlot",
                                               "actionViewItemChosen",
                                               [list.currentIndex])
        Keys.onDownPressed: list.incrementCurrentIndex()
        Keys.onUpPressed: list.decrementCurrentIndex()
      }
      ButtonWidget {
        id: button
        text: dataViewButtonText
        onClicked: core.OnUserAction("viewSlot", "actionViewItemChosen",
                                     [list.currentIndex])
      }
    }
  }
  PaneWidget {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: colorBgDark
    ListView {
      id: list
      anchors.fill: parent
      clip: true
      boundsBehavior: ListView.StopAtBounds
      model: dataViewListModel
      delegate: TextWidget {
        text: title
        width: list.width
        padding: basePadding
        highlight: ListView.isCurrentItem
        background: Rectangle {
          anchors.fill: parent
          color: parent.highlight ? colorBgMedium : "transparent"
        }
        MouseArea {
          anchors.fill: parent
          onClicked: {
            list.currentIndex = index;
            core.OnUserAction("viewSlot", "actionViewItemChosen",
                              [list.currentIndex]);
          }
        }
      }
    }
  }
}
