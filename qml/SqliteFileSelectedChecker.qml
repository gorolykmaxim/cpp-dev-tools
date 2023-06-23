import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

Cdt.Pane {
  id: root
  property bool isSelected: sqliteSystem.isFileSelected
  signal displayOpenSqliteFileView()
  visible: !isSelected
  padding: Theme.basePadding
  color: Theme.colorBorder
  focus: visible
  RowLayout {
    spacing: Theme.basePadding
    Cdt.Text {
      id: err
      text: "No SQLite database is opened/selected."
      color: "red"
    }
    Cdt.Button {
      id: fixBtn
      text: "Open"
      focus: true
      onClicked: displayOpenSqliteFileView()
    }
  }
}
