import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
  anchors.fill: parent
  spacing: 0

  function execCommand(eventType) {
    dialog.reject();
    core.OnAction(eventType, []);
  }

  PaneWidget {
    Layout.fillWidth: true
    padding: basePadding
    focus: true
    TextFieldWidget {
      width: parent.width
      text: dFilter || ""
      placeholderText: "Search command by name"
      focus: true
      onDisplayTextChanged: core.OnAction("daFilterChanged", [displayText])
      Keys.onReturnPressed: execCommand(list.currentItem.itemEventType)
      Keys.onEnterPressed: execCommand(list.currentItem.itemEventType)
      Keys.onDownPressed: list.incrementCurrentIndex()
      Keys.onUpPressed: list.decrementCurrentIndex()
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
      model: dCommands
      delegate: Rectangle {
        property var isSelected: ListView.isCurrentItem
        property var itemEventType: eventType
        width: list.width
        height: row.height
        color: isSelected ? colorBgMedium : "transparent"
        RowLayout {
          id: row
          width: parent.width
          Column {
            Layout.margins: basePadding
            TextWidget {
              text: name
              highlight: isSelected
            }
            TextWidget {
              text: group
              color: colorSubText
            }
          }
          TextWidget {
            Layout.alignment: Qt.AlignRight
            Layout.margins: basePadding
            text: shortcut
            color: colorSubText
          }
        }
        MouseArea {
          anchors.fill: parent
          onClicked: {
            list.currentIndex = index;
            execCommand(eventType);
          }
        }
      }
    }
  }
}
