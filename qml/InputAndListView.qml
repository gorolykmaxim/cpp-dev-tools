import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
  SystemPalette {id: palette; colorGroup: SystemPalette.Active}
  anchors.fill: parent
  RowLayout {
    TextField {
      id: input
      placeholderText: "Enter something here"
      focus: true
      Layout.fillWidth: true
      KeyNavigation.right: button
      Keys.onDownPressed: list.incrementCurrentIndex()
      Keys.onUpPressed: list.decrementCurrentIndex()
    }
    Button {
      id: button
      text: "Open"
      KeyNavigation.left: input
      Keys.onReturnPressed: clicked()
      Keys.onEnterPressed: clicked()
    }
  }
  ListView {
    id: list
    clip: true
    Layout.fillWidth: true
    Layout.fillHeight: true
    model: ListModel {
      ListElement {
        title: "../"
      }
      ListElement {
        title: "foo/"
      }
      ListElement {
        title: "bar/"
      }
      ListElement {
        title: "baz"
      }
    }
    delegate: Label {
      property var highlight: ListView.isCurrentItem
      text: title
      width: parent.width
      background: Rectangle {
        anchors.fill: parent
        color: highlight ? palette.highlight : "transparent"
      }
    }
  }
}
