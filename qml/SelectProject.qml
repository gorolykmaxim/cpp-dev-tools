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
  ProjectController {
    id: controller
    onSelectProject: root.sourceComponent = selectProjectView
  }
  Component {
    id: selectProjectView
    ColumnLayout {
      anchors.fill: parent
      spacing: 0
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
            onEnterPressed: projectList.ifCurrentItem('idx', controller.openProject)
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
          onItemLeftClicked: projectList.ifCurrentItem('idx', controller.openProject)
          onItemRightClicked: contextMenu.open()
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
          text: "Remove From List"
          enabled: input.activeFocus
          shortcut: "Alt+Shift+D"
          onTriggered: projectList.ifCurrentItem('idx', controller.deleteProject)
        }
      }
    }
  }
  Component {
    id: chooseProjectView
    Cdt.ChooseFile {
      onFileChosen: (file) => controller.openNewProject(file)
      onCancelled: root.sourceComponent = selectProjectView
    }
  }
}
