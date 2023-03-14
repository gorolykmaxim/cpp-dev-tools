import QtQuick
import "." as Cdt
import cdt

Cdt.ChooseTask {
  onTaskSelected: (cmd) => controller.ExecuteTask(cmd)
  RunTaskController {
    id: controller
  }
}
