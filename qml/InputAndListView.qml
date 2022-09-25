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
      TextWidget {
        text: dataInputLabel
      }
      TextFieldWidget {
        id: input
        text: dataInputValue
        focus: true
        Layout.fillWidth: true
        KeyNavigation.right: button
        onDisplayTextChanged: core.OnUserAction("inputValueChanged",
                                                [displayText])
        Keys.onReturnPressed: core.OnUserAction("enterPressed", [])
        Keys.onEnterPressed: core.OnUserAction("enterPressed", [])
        Keys.onDownPressed: list.incrementCurrentIndex()
        Keys.onUpPressed: list.decrementCurrentIndex()
      }
      ButtonWidget {
        id: button
        text: dataButtonText
        onClicked: core.OnUserAction("enterPressed", [])
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
      model: dataListModel
      onCurrentIndexChanged: core.OnUserAction("itemSelected", [currentIndex])
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
            core.OnUserAction("enterPressed", []);
          }
        }
      }
    }
  }
}
