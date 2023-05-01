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
      Component.onCompleted: controller.displayList()
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
            placeholderText: "Search database"
            Layout.fillWidth: true
            KeyNavigation.right: addBtn
            list: databaseList
            listModel: controller.databases
            focus: true
            onEnterPressed: databaseList.ifCurrentItem('idx', controller.selectDatabase)
          }
          Cdt.Button {
            id: addBtn
            text: "Add Database"
            onClicked: root.sourceComponent = openDatabaseView
          }
        }
      }
      Cdt.Pane {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: Theme.colorBgDark
        Cdt.TextList {
          id: databaseList
          model: controller.databases
          anchors.fill: parent
          onItemLeftClicked: ifCurrentItem('idx', controller.selectDatabase)
          onItemRightClicked: contextMenu.open()
          onItemSelected: ifCurrentItem('idx', controller.highlightDatabase)
        }
      }
      Menu {
        id: contextMenu
        MenuItem {
          text: "Select"
          enabled: input.activeFocus && (databaseList.currentItem?.itemModel?.existsOnDisk || false)
          shortcut: "Enter"
          onTriggered: databaseList.ifCurrentItem('idx', controller.selectDatabase)
        }
        MenuItem {
          text: "Change Path"
          enabled: input.activeFocus
          shortcut: "Alt+E"
          onTriggered: root.sourceComponent = updateDatabasePathView
        }
        MenuItem {
          text: "Remove From List"
          enabled: input.activeFocus
          shortcut: "Alt+Shift+D"
          onTriggered: databaseList.ifCurrentItem('idx', controller.removeDatabase)
        }
      }
    }
  }
  Component {
    id: openDatabaseView
    Cdt.OpenSqliteFile {
      onDatabaseOpened: root.sourceComponent = listView
      onCancelled: root.sourceComponent = listView
    }
  }
  Component {
    id: updateDatabasePathView
    Cdt.OpenSqliteFile {
      title: "Change SQLite Database's Path"
      fileId: controller.highlightedFileId
      onDatabaseOpened: root.sourceComponent = listView
      onCancelled: root.sourceComponent = listView
    }
  }
}
