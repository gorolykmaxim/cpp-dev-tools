import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

Loader {
  id: root
  anchors.fill: parent
  sourceComponent: view
  focus: true
  RunCmakeController {
    id: controller
    onDisplayView: sourceComponent = view
    onDisplayChooseBuildFolderView: sourceComponent = chooseBuildFolderView
    onDisplayChooseSourceFolderView: sourceComponent = chooseSourceFolderView
  }
  Component {
    id: view
    Cdt.Pane {
      anchors.fill: parent
      focus: true
      ColumnLayout {
        anchors.centerIn: parent
        width: Theme.centeredViewWidth
        spacing: Theme.basePadding
        Cdt.Text {
          text: "Choose source folder and build folder to run CMake in:"
        }
        RowLayout {
          Layout.fillWidth: true
          spacing: Theme.basePadding
          Cdt.Text {
            text: controller.sourceFolder || "Source folder path"
            elide: Text.ElideLeft
            clip: true
            color: Theme.colorPlaceholder
            Layout.fillWidth: true
          }
          Cdt.Button {
            id: chooseSourceBtn
            text: "Choose"
            focus: true
            onClicked: controller.displayChooseSourceFolder()
            KeyNavigation.down: chooseBuildBtn
          }
        }
        RowLayout {
          Layout.fillWidth: true
          spacing: Theme.basePadding
          Cdt.Text {
            text: controller.buildFolder || "Build folder path"
            elide: Text.ElideLeft
            clip: true
            color: Theme.colorPlaceholder
            Layout.fillWidth: true
          }
          Cdt.Button {
            id: chooseBuildBtn
            text: "Choose"
            onClicked: controller.displayChooseBuildFolder()
            KeyNavigation.down: runBtn
          }
        }
        Cdt.Button {
          id: runBtn
          enabled: controller.sourceFolder && controller.buildFolder
          Layout.alignment: Qt.AlignRight
          text: "Run CMake"
          onClicked: controller.runCmake()
        }
      }
    }
  }
  Component {
    id: chooseBuildFolderView
    Cdt.ChooseFile {
      chooseFolder: true
      startingFolder: controller.rootFolder + '/'
      onFileChosen: file => controller.setBuildFolder(file)
      onCancelled: controller.back()
    }
  }
  Component {
    id: chooseSourceFolderView
    Cdt.ChooseFile {
      chooseFolder: true
      startingFolder: controller.rootFolder + '/'
      onFileChosen: file => controller.setSourceFolder(file)
      onCancelled: controller.back()
    }
  }
}
