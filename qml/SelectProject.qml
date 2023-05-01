import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.platform
import cdt
import "." as Cdt

Loader {
  id: root
  anchors.fill: parent
  focus: true
  sourceComponent: selectProjectView
  ProjectController {
    id: controller
    onDisplayList: root.sourceComponent = selectProjectView
    onDisplayOpenNewProjectView: root.sourceComponent = chooseProjectView
    onDisplayUpdateExistingProjectView: root.sourceComponent = updateProjectPathView
  }
  Component {
    id: selectProjectView
    ColumnLayout {
      anchors.fill: parent
      spacing: 0
      Component.onCompleted: controller.displayProjects()
      Cdt.Pane {
        Layout.fillWidth: true
        color: Theme.colorBgMedium
        padding: Theme.basePadding
        focus: true
        RowLayout {
          anchors.fill: parent
          Cdt.ListSearch {
            id: input
            placeholderText: "Search project"
            Layout.fillWidth: true
            KeyNavigation.right: button
            list: projectList
            listModel: controller.projects
            focus: true
            onEnterPressed: controller.openSelectedProject()
          }
          Cdt.Button {
            id: button
            text: "New Project"
            onClicked: controller.openNewProject()
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
          onItemLeftClicked: controller.openSelectedProject()
          onItemRightClicked: contextMenu.open()
          onItemSelected: ifCurrentItem('idx', controller.selectProject)
        }
      }
      Menu {
        id: contextMenu
        MenuItem {
          text: "Open"
          enabled: input.activeFocus && (projectList.currentItem?.itemModel?.existsOnDisk || false)
          onTriggered: input.enterPressed()
        }
        MenuItem {
          text: "Change Path"
          enabled: input.activeFocus
          shortcut: "Alt+E"
          onTriggered: controller.updateProjectPath()
        }
        MenuItem {
          text: "Remove From List"
          enabled: input.activeFocus
          shortcut: "Alt+Shift+D"
          onTriggered: controller.deleteSelectedProject()
        }
      }
    }
  }
  Component {
    id: chooseProjectView
    Cdt.ChooseFile {
      chooseFolder: true
      onFileChosen: (file) => controller.openNewProject(file)
      onCancelled: root.sourceComponent = selectProjectView
    }
  }
  Component {
    id: updateProjectPathView
    Cdt.ChooseFile {
      chooseFolder: true
      onFileChosen: (file) => controller.updateSelectedProjectPath(file)
      onCancelled: root.sourceComponent = selectProjectView
    }
  }
}
