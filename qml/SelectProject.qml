import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.platform
import cdt
import "." as Cdt

Loader {
  id: root
  anchors.fill: parent
  ProjectController {
    id: controller
    onSelectProject: root.sourceComponent = selectProjectView
  }
  Component {
    id: selectProjectView
    ColumnLayout {
      Component.onCompleted: {
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
            placeholderText: "Search project"
            Layout.fillWidth: true
            KeyNavigation.right: button
            list: projectList
            listModel: controller.projects
            onEnterPressed: projectList.ifCurrentItem('idx', controller.OpenProject)
            Shortcut {
              id: shortcutRemoveFromList
              enabled: input.activeFocus
              sequence: "Ctrl+Shift+D"
              onActivated: projectList.ifCurrentItem('idx', controller.DeleteProject)
            }
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
          onItemLeftClicked: projectList.ifCurrentItem('idx', controller.OpenProject)
          onItemRightClicked: contextMenu.open()
        }
      }
      Menu {
        id: contextMenu
        MenuItem {
          text: "Open"
          onTriggered: input.enterPressed()
        }
        MenuItem {
          text: "Remove From List"
          onTriggered: shortcutRemoveFromList.activated()
        }
      }
    }
  }
  Component {
    id: chooseProjectView
    Cdt.ChooseFile {
      onFileChosen: (file) => controller.OpenNewProject(file)
      onCancelled: root.sourceComponent = selectProjectView
    }
  }
}
