import "." as Cdt
import cdt

Cdt.TestExecution {
  taskName: model.taskName
  testModel: model
  QTestExecutionModel {
    id: model
  }
}
