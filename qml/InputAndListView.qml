import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
  SystemPalette {id: palette; colorGroup: SystemPalette.Active}
  Component.onCompleted: {
    list.model.onModelReset.connect(() => list.currentIndex = 0);
  }
  anchors.fill: parent
  RowLayout {
    spacing: globalPadding * 2
    Label {
      text: inputAndListDataInputLabel
    }
    TextField {
      id: input
      text: inputAndListDataInputValue
      focus: true
      Layout.fillWidth: true
      KeyNavigation.right: button
      onDisplayTextChanged: core.OnUserAction("inputValueChanged", [displayText])
      Keys.onReturnPressed: core.OnUserAction("enterPressed", [])
      Keys.onEnterPressed: core.OnUserAction("enterPressed", [])
      Keys.onDownPressed: list.incrementCurrentIndex()
      Keys.onUpPressed: list.decrementCurrentIndex()
    }
    Button {
      id: button
      text: inputAndListDataButtonText
      enabled: inputAndListDataIsButtonEnabled
      KeyNavigation.left: input
      Keys.onReturnPressed: clicked()
      Keys.onEnterPressed: clicked()
      onClicked: core.OnUserAction("enterPressed", [])
    }
  }
  ListView {
    id: list
    clip: true
    Layout.fillWidth: true
    Layout.fillHeight: true
    model: inputAndListDataListModel
    onCurrentIndexChanged: core.OnUserAction("itemSelected", [currentIndex])
    delegate: Label {
      property var highlight: ListView.isCurrentItem
      text: title
      width: list.width
      background: Rectangle {
        anchors.fill: parent
        color: highlight ? palette.highlight : "transparent"
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
