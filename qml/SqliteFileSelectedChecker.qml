import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

RowLayout {
  id: root
  property bool isSelected: sqliteSystem.isFileSelected
  signal displayOpenSqliteFileView()
  visible: !isSelected
  Cdt.Text {
    id: err
    text: "No SQLite database is opened/selected."
    color: "red"
    Layout.margins: Theme.basePadding
  }
  Cdt.Button {
    id: fixBtn
    text: "Open"
    focus: root.visible
    Layout.margins: Theme.basePadding
    onClicked: displayOpenSqliteFileView()
  }
}
