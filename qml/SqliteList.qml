import QtQuick
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt

Loader {
  id: root
  focus: true
  anchors.fill: parent
  sourceComponent: listView
  SqliteListController {
    id: controller
  }
  Component {
    id: listView
    ColumnLayout {
      Component.onCompleted: controller.displayDatabaseList()
      anchors.fill: parent
      spacing: 0
      Cdt.Pane {
        Layout.fillWidth: true
        padding: Theme.basePadding
        focus: true
        RowLayout {
          anchors.fill: parent
          Cdt.ListSearch {
            id: input
            placeholderText: "Search database"
            Layout.fillWidth: true
            KeyNavigation.right: addBtn
            list: databaseList
            listModel: controller.databases
            focus: true
            onEnterPressed: controller.useSelectedDatabase()
          }
          Cdt.Button {
            id: addBtn
            text: "Add Database"
            onClicked: root.sourceComponent = openDatabaseView
          }
        }
      }
      Cdt.TextList {
        id: databaseList
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: controller.databases
        onItemLeftClicked: controller.useSelectedDatabase()
        onItemRightClicked: contextMenu.open()
      }
      Menu {
        id: contextMenu
        MenuItem {
          text: "Select"
          enabled: (input.activeFocus || databaseList.activeFocus) && (databaseList.currentItem?.itemModel?.existsOnDisk || false)
          shortcut: "Enter"
          onTriggered: controller.useSelectedDatabase()
        }
        MenuItem {
          text: "Change Path"
          enabled: (input.activeFocus || databaseList.activeFocus) && controller.isDatabaseSelected
          shortcut: gSC("SqliteList", "Change Path")
          onTriggered: root.sourceComponent = updateDatabasePathView
        }
        MenuItem {
          text: "Remove From List"
          enabled: (input.activeFocus || databaseList.activeFocus) && controller.isDatabaseSelected
          shortcut: gSC("SqliteList", "Remove From List")
          onTriggered: controller.removeSelectedDatabase()
        }
      }
    }
  }
  Component {
    id: openDatabaseView
    Cdt.OpenSqliteFile {
      title: "Open SQLite Database"
      onDatabaseOpened: root.sourceComponent = listView
      onCancelled: root.sourceComponent = listView
    }
  }
  Component {
    id: updateDatabasePathView
    Cdt.OpenSqliteFile {
      title: "Change SQLite Database's Path"
      fileId: controller.selectedDatabaseFileId
      onDatabaseOpened: root.sourceComponent = listView
      onCancelled: root.sourceComponent = listView
    }
  }
}
