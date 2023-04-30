import "." as Cdt
import cdt

Cdt.ChooseFile {
  id: root
  signal databaseOpened()
  allowCreating: false
  onFileChosen: (file) => controller.openDatabase(file)
  OpenSqliteFileController {
    id: controller
    onDatabaseOpened: root.databaseOpened()
  }
}
