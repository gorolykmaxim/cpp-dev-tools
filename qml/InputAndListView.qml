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
    Label {
      padding: globalPadding
      text: inputAndListDataInputLabel
    }
    TextField {
      id: input
      text: inputAndListDataInputValue
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
    Button {
      id: button
      highlighted: true
      text: inputAndListDataButtonText
      Keys.onReturnPressed: clicked()
      Keys.onEnterPressed: clicked()
      onClicked: core.OnUserAction("enterPressed", [])
    }
  }
  Label {
    leftPadding: globalPadding
    rightPadding: globalPadding
    text: inputAndListDataError
    visible: inputAndListDataError.length > 0
    color: "red"
    Layout.fillWidth: true
    wrapMode: Label.WordWrap
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
      padding: globalPadding
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
