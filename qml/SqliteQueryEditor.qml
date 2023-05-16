import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "." as Cdt
import cdt

Loader {
  id: root
  anchors.fill: parent
  focus: true
  sourceComponent: view
  Component {
    id: view
    ColumnLayout {
      spacing: 0
      Cdt.SqliteFileSelectedChecker {
        id: dbFile
        onDisplayOpenSqliteFileView: root.sourceComponent = openView
      }
      Cdt.Pane {
        Layout.fillWidth: true
        Layout.fillHeight: true
        focus: true
        color: Theme.colorBgDark
        SqliteQueryEditorController {
          id: controller
        }
        SplitView {
          id: splitView
          anchors.fill: parent
          handle: Cdt.SplitViewHandle {
            viewId: "SqliteQueryEditor"
            view: splitView
          }
          Cdt.TextArea {
            SplitView.minimumWidth: 300
            SplitView.fillHeight: true
            focus: true
            detectFileLinks: false
            enabled: dbFile.isSelected
            searchable: true
            text: controller.query
            placeholderText: "Enter SQL queries here"
            innerPadding: Theme.basePadding
            formatter: controller.formatter
            onDisplayTextChanged: controller.query = displayText
            KeyNavigation.right: tableView
            onCtrlEnterPressed: controller.executeQuery(getText(), effectiveCursorPosition)
          }
          Cdt.TableView {
            id: tableView
            SplitView.fillWidth: true
            SplitView.fillHeight: true
            model: controller.model
          }
        }
      }
    }
  }
  Component {
    id: openView
    Cdt.OpenSqliteFile {
      title: "Open SQLite Database"
      onDatabaseOpened: root.sourceComponent = view
      onCancelled: root.sourceComponent = view
    }
  }
}
