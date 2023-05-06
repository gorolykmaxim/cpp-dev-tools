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
    onDisplayProjectListView: root.sourceComponent = selectProjectView
    onDisplayOpenProjectView: root.sourceComponent = chooseProjectView
    onDisplayChangeProjectPathView: root.sourceComponent = updateProjectPathView
  }
  Component {
    id: selectProjectView
    ColumnLayout {
      anchors.fill: parent
      spacing: 0
      Component.onCompleted: controller.displayProjectList()
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
            onClicked: controller.displayOpenProject()
          }
        }
      }
      Cdt.TextList {
        id: projectList
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: controller.projects
        placeholderText: "Open a project by clicking on '" + button.text + "' button"
        onItemLeftClicked: controller.openSelectedProject()
        onItemRightClicked: contextMenu.open()
        onItemSelected: ifCurrentItem('idx', controller.selectProject)
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
          enabled: input.activeFocus && controller.isProjectSelected
          shortcut: "Alt+E"
          onTriggered: controller.displayChangeProjectPath()
        }
        MenuItem {
          text: "Remove From List"
          enabled: input.activeFocus && controller.isProjectSelected
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
      onFileChosen: (file) => controller.openProject(file)
      onCancelled: controller.displayProjectList()
    }
  }
  Component {
    id: updateProjectPathView
    Cdt.ChooseFile {
      chooseFolder: true
      onFileChosen: (file) => controller.changeSelectedProjectPath(file)
      onCancelled: controller.displayProjectList()
    }
  }
}
