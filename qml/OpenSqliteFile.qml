import QtQuick
import "." as Cdt
import cdt

Cdt.ChooseFile {
  id: root
  property alias fileId: controller.fileId
  property alias title: controller.title
  signal databaseOpened()
  allowCreating: false
  onFileChosen: (file) => controller.openDatabase(file)
  OpenSqliteFileController {
    id: controller
    onDatabaseOpened: root.databaseOpened()
  }
}
