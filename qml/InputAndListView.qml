import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
  Component.onCompleted: {
    list.model.onModelReset.connect(() => list.currentIndex = 0);
  }
  anchors.fill: parent
  RowLayout {
    Label {
      padding: globalPadding
      text: dataInputLabel
    }
    TextField {
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
      highlighted: true
      text: dataButtonText
      onClicked: core.OnUserAction("enterPressed", [])
    }
  }
  ListView {
    id: list
    clip: true
    Layout.fillWidth: true
    Layout.fillHeight: true
    model: dataListModel
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
