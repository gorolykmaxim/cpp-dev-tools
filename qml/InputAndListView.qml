import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
  SystemPalette {id: palette; colorGroup: SystemPalette.Active}
  anchors.fill: parent
  RowLayout {
    spacing: globalPadding * 2
    Label {
      text: inputAndListData.inputLabel
    }
    TextField {
      id: input
      text: inputAndListData.inputValue
      focus: true
      Layout.fillWidth: true
      KeyNavigation.right: button
      onDisplayTextChanged: inputAndListData.OnValueChanged(displayText)
      Keys.onReturnPressed: inputAndListData.OnEnter()
      Keys.onEnterPressed: inputAndListData.OnEnter()
      Keys.onDownPressed: list.incrementCurrentIndex()
      Keys.onUpPressed: list.decrementCurrentIndex()
    }
    Button {
      id: button
      text: inputAndListData.buttonText
      enabled: inputAndListData.isButtonEnabled
      KeyNavigation.left: input
      Keys.onReturnPressed: clicked()
      Keys.onEnterPressed: clicked()
      onClicked: inputAndListData.OnEnter()
    }
  }
  ListView {
    id: list
    clip: true
    Layout.fillWidth: true
    Layout.fillHeight: true
    model: inputAndListData.listModel
    onCurrentIndexChanged: inputAndListData.OnListItemSelected(currentIndex)
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
          inputAndListData.OnEnter();
        }
      }
    }
  }
}
