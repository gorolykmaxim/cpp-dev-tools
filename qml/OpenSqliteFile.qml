import QtQuick
import "." as Cdt
import cdt

Cdt.ChooseFile {
  id: root
  property alias fileId: controller.fileId
  property string title: "Open SQLite Database"
  signal databaseOpened()
  allowCreating: false
  onFileChosen: (file) => controller.openDatabase(file)
  Component.onCompleted: viewSystem.windowTitle = title
  OpenSqliteFileController {
    id: controller
    onDatabaseOpened: root.databaseOpened()
  }
}
