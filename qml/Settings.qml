import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
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
    onOpenDocumentationFoldersEditor: sourceComponent = documentationFolderEditorView
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
              Layout.alignment: Qt.AlignHCenter
              onClicked: controller.configureExternalSearchFolders()
              KeyNavigation.down: configureDocumentationFoldersBtn
            }
            Cdt.Button {
              id: configureDocumentationFoldersBtn
              text: "Configure Documentation Folders"
              Layout.columnSpan: 2
              Layout.alignment: Qt.AlignHCenter
              onClicked: controller.configureDocumentationFolders()
              KeyNavigation.down: terminalPriorityList
            }
            Cdt.Text {
              text: "Terminal Priority"
              Layout.minimumWidth: 200
              Layout.alignment: Qt.AlignTop
            }
            Rectangle {
              Layout.fillWidth: true
              height: 80
              color: Theme.colorBgDark
              border.color: Theme.colorBgBlack
              border.width: 1
              radius: Theme.baseRadius
              Cdt.TextList {
                id: terminalPriorityList
                anchors.fill: parent
                anchors.margins: 1
                color: "transparent"
                model: controller.terminals
                highlightCurrentItemWithoutFocus: false
                onItemRightClicked: contextMenu.open()
                KeyNavigation.down: saveBtn
                Menu {
                  id: contextMenu
                  MenuItem {
                    text: "Move Up"
                    enabled: terminalPriorityList.activeFocus
                    shortcut: "Shift+Up"
                    onTriggered: controller.moveSelectedTerminal(true)
                  }
                  MenuItem {
                    text: "Move Down"
                    enabled: terminalPriorityList.activeFocus
                    shortcut: "Shift+Down"
                    onTriggered: controller.moveSelectedTerminal(false)
                  }
                }
              }
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
      onFolderAdded: folder => controller.externalSearchFolders.addFolder(folder)
      onFolderRemoved: folder => controller.externalSearchFolders.removeFolder(folder)
      onBack: controller.goToSettings()
    }
  }
  Component {
    id: documentationFolderEditorView
    Cdt.FolderList {
      foldersModel: controller.documentationFolders
      onFolderAdded: folder => controller.documentationFolders.addFolder(folder)
      onFolderRemoved: folder => controller.documentationFolders.removeFolder(folder)
      onBack: controller.goToSettings()
    }
  }
}
