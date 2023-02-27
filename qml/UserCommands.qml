import QtQuick
import cdt

UserCommandController {
  Component.onCompleted: {
    RegisterCommand("General", "Execute Command", "Ctrl+P",
                    () => searchUserCommandDialog.display());
    RegisterCommand("Task", "Run Task", "Ctrl+R",
                    () => currentView.source = "RunTask.qml");
    Commit();
  }
}
