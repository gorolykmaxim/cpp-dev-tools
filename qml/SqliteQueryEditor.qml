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
        Layout.fillWidth: true
        onDisplayOpenSqliteFileView: root.sourceComponent = openView
      }
      SplitView {
        id: splitView
        Layout.fillWidth: true
        Layout.fillHeight: true
        handle: Cdt.SplitViewHandle {
          viewId: "SqliteQueryEditor"
          view: splitView
        }
        SqliteQueryEditorController {
          id: controller
        }
        Cdt.SmallTextArea {
          SplitView.minimumWidth: 300
          SplitView.fillHeight: true
          focus: true
          enabled: dbFile.isSelected
          text: controller.query
          placeholderText: "Enter SQL queries here"
          formatter: controller.formatter
          onTextChanged: controller.query = text
          KeyNavigation.right: tableView
          onCtrlEnterPressed: controller.executeQuery(text, effectiveCursorPosition)
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
  Component {
    id: openView
    Cdt.OpenSqliteFile {
      title: "Open SQLite Database"
      onDatabaseOpened: root.sourceComponent = view
      onCancelled: root.sourceComponent = view
    }
  }
}
