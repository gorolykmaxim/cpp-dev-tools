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
        text: vFilter || ""
        placeholderText: "Search project by name"
        focus: true
        Layout.fillWidth: true
        KeyNavigation.right: button
        onDisplayTextChanged: core.OnAction("vaFilterChanged", [displayText])
        Keys.onReturnPressed: core.OnAction("vaProjectSelected",
                                            [list.currentItem.itemId])
        Keys.onEnterPressed: core.OnAction("vaProjectSelected",
                                           [list.currentItem.itemId])
        Keys.onDownPressed: list.incrementCurrentIndex()
        Keys.onUpPressed: list.decrementCurrentIndex()
      }
      ButtonWidget {
        id: button
        text: "New Project"
        onClicked: core.OnAction("vaNewProject", [])
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
      // model: ListModel {
      //   ListElement {
      //     idx: 1
      //     title: "Project 1"
      //     subTitle: "~/dev/Project 1"
      //   }
      // }
      model: vProjects
      delegate: Rectangle {
        property var isSelected: ListView.isCurrentItem
        property var itemId: idx
        width: column.width
        height: column.height
        color: isSelected ? colorBgMedium : "transparent"
        Column {
          id: column
          padding: basePadding
          TextWidget {
            text: title
            width: list.width
            highlight: isSelected
          }
          TextWidget {
            text: subTitle
            width: list.width
            color: colorSubText
          }
        }
        MouseArea {
          anchors.fill: parent
          onClicked: {
            list.currentIndex = index;
            core.OnAction("vaProjectSelected", [list.currentItem.itemId]);
          }
        }
      }
    }
  }
}