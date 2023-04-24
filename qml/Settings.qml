import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "." as Cdt
import cdt
import "Common.js" as Common

Loader {
  id: root
  focus: true
  anchors.fill: parent
  sourceComponent: settingsView
  SettingsController {
    id: controller
    onOpenExternalSearchFoldersEditor: sourceComponent = externalSearchFolderEditorView
    onOpenSettings: sourceComponent = settingsView
  }
  Component {
    id: settingsView
    Cdt.Pane {
      anchors.fill: parent
      focus: true
      color: Theme.colorBgMedium
      padding: Theme.basePadding
      ColumnLayout {
        id: settingsRoot
        anchors.fill: parent
        ScrollView {
          focus: true
          Layout.fillWidth: true
          Layout.fillHeight: true
          GridLayout {
            width: settingsRoot.width
            columns: 2
            Cdt.Text {
              text: "Open In Editor Command"
              Layout.minimumWidth: 200
            }
            Cdt.TextField {
              text: controller.settings.openInEditorCommand
              onDisplayTextChanged: controller.settings.openInEditorCommand = displayText
              focus: true
              Layout.fillWidth: true
              KeyNavigation.down: taskHistoryLimitInput
            }
            Cdt.Text {
              text: "Maximum Task History Size"
              Layout.minimumWidth: 200
            }
            Cdt.TextField {
              id: taskHistoryLimitInput
              text: controller.settings.taskHistoryLimit
              validator: IntValidator {
                bottom: 0
                top: Common.MAX_INT
              }
              onDisplayTextChanged: controller.settings.taskHistoryLimit = displayText
              Layout.fillWidth: true
              KeyNavigation.down: configureExternalSearchFoldersBtn
            }
            Cdt.Button {
              id: configureExternalSearchFoldersBtn
              text: "Configure External Search Folders"
              Layout.columnSpan: 2
              onClicked: controller.configureExternalSearchFolders()
              KeyNavigation.down: saveBtn
            }
          }
        }
        Cdt.Button {
          id: saveBtn
          Layout.alignment: Qt.AlignRight
          text: "Save"
          onClicked: controller.save()
        }
      }
    }
  }
  Component {
    id: externalSearchFolderEditorView
    Cdt.FolderList {
      foldersModel: controller.externalSearchFolders
      onFolderAdded: folder => controller.addExternalSearchFolder(folder)
      onFolderRemoved: folder => controller.removeExternalSearchFolder(folder)
      onCancel: controller.goToSettings()
    }
  }
}

