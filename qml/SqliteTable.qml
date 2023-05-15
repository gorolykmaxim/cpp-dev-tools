import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

Loader {
  id: root
  anchors.fill: parent
  focus: true
  SqliteTableController {
    id: controller
    onDisplayTableView: root.sourceComponent = tableView
  }
  sourceComponent: tableListView
  Component {
    id: tableListView
    ColumnLayout {
      spacing: 0
      anchors.fill: parent
      Component.onCompleted: controller.displayTableList()
      Cdt.SqliteFileSelectedChecker {
        id: dbFile
        onDisplayOpenSqliteFileView: root.sourceComponent = openView
      }
      Cdt.SearchableTextList {
        Layout.fillWidth: true
        Layout.fillHeight: true
        focus: dbFile.isSelected
        searchableModel: controller.tables
        searchPlaceholderText: "Search table"
        showPlaceholder: controller.status
        placeholderText: controller.status
        placeholderColor: controller.statusColor
        onItemSelected: item => controller.displaySelectedTable()
      }
    }
  }
  Component {
    id: tableView
    Cdt.Pane {
      anchors.fill: parent
      focus: true
      color: Theme.colorBgDark
      ColumnLayout {
        anchors.fill: parent
        spacing: 0
        Keys.onPressed: function(e) {
          switch (e.key) {
          case Qt.Key_Escape:
            backBtn.clicked()
            break;
          case Qt.Key_F5:
          case Qt.Key_Return:
          case Qt.Key_Enter:
            reloadBtn.clicked()
            break;
          }
        }
        Cdt.Pane {
          color: Theme.colorBgMedium
          padding: Theme.basePadding
          Layout.fillWidth: true
          RowLayout {
            anchors.fill: parent
            spacing: Theme.basePadding
            Cdt.IconButton {
              id: backBtn
              buttonIcon: "keyboard_backspace"
              onClicked: root.sourceComponent = tableListView
              KeyNavigation.right: reloadBtn
              KeyNavigation.down: table
            }
            Cdt.IconButton {
              id: reloadBtn
              buttonIcon: "loop"
              onClicked: controller.load()
              KeyNavigation.right: whereInput
              KeyNavigation.down: table
            }
            Cdt.Text {
              text: "WHERE"
            }
            Cdt.TextField {
              id: whereInput
              onDisplayTextChanged: controller.setWhere(displayText)
              Layout.fillWidth: true
              KeyNavigation.right: orderByInput
              KeyNavigation.down: table
            }
            Cdt.Text {
              text: "ORDER BY"
            }
            Cdt.TextField {
              id: orderByInput
              onDisplayTextChanged: controller.setOrderBy(displayText)
              Layout.minimumWidth: 150
              Layout.maximumWidth: 150
              KeyNavigation.right: limitInput
              KeyNavigation.down: table
            }
            Cdt.Text {
              text: "LIMIT"
            }
            Cdt.TextField {
              id: limitInput
              text: controller.limit
              onDisplayTextChanged: controller.setLimit(displayText)
              validator: IntValidator {
                bottom: 0
                top: Common.MAX_INT
              }
              Layout.minimumWidth: 50
              Layout.maximumWidth: 50
              KeyNavigation.right: offsetInput
              KeyNavigation.down: table
            }
            Cdt.Text {
              text: "OFFSET"
            }
            Cdt.TextField {
              id: offsetInput
              onDisplayTextChanged: controller.setOffset(displayText)
              validator: IntValidator {
                bottom: 0
                top: Common.MAX_INT
              }
              Layout.minimumWidth: 50
              Layout.maximumWidth: 50
              KeyNavigation.down: table
            }
          }
        }
        Cdt.TableView {
          id: table
          Layout.fillWidth: true
          Layout.fillHeight: true
          model: controller.table
          focus: true
          showPlaceholder: controller.status
          placeholderText: controller.status
          placeholderColor: controller.statusColor
        }
      }
    }
  }
  Component {
    id: openView
    Cdt.OpenSqliteFile {
      title: "Open SQLite Database"
      onDatabaseOpened: root.sourceComponent = tableListView
      onCancelled: root.sourceComponent = tableListView
    }
  }
}
