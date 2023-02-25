import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.platform
import cdt
import "." as Cdt

Loader {
  id: root
  anchors.fill: parent
  sourceComponent: selectProjectView
  ProjectController {
    id: controller
  }
  Component {
    id: selectProjectView
    ColumnLayout {
      Component.onCompleted: {
        appWindow.title = "Open Project";
        input.forceActiveFocus();
      }
      anchors.fill: parent
      spacing: 0
      Cdt.Pane {
        Layout.fillWidth: true
        color: Theme.colorBgMedium
        padding: Theme.basePadding
        RowLayout {
          anchors.fill: parent
          Cdt.ListSearch {
            id: input
            text: controller.projects.filter
            placeholderText: "Search project"
            Layout.fillWidth: true
            KeyNavigation.right: button
            list: projectList
            onDisplayTextChanged: controller.projects.filter = displayText
            onEnterPressed: console.log("ENTER")
            onCtrlEnterPressed: console.log("CTRL ENTER")
          }
          Cdt.Button {
            id: button
            text: "New Project"
            onClicked: root.sourceComponent = chooseProjectView
          }
        }
      }
      Cdt.Pane {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: Theme.colorBgDark
        Cdt.TextList {
          id: projectList
          anchors.fill: parent
          model: controller.projects
          onItemLeftClicked: console.log("LEFT CLICK")
          onItemRightClicked: contextMenu.open()
        }
      }
      Menu {
        id: contextMenu
        MenuItem {
          text: "Open"
          shortcut: "Enter"
          onTriggered: console.log(text)
        }
        MenuItem {
          text: "Remove From List"
          shortcut: "Ctrl+Shift+D"
          onTriggered: controller.DeleteProject(projectList.currentItem.itemModel.idx)
        }
      }
    }
  }
  Component {
    id: chooseProjectView
    Cdt.ChooseFile {
      title: "Open Project"
      onFileChosen: (file) => console.log("FILE CHOSEN " + file)
      onCancelled: root.sourceComponent = selectProjectView
    }
  }
}