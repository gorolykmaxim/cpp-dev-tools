import QtQuick
import cdt

UserCommandController {
  Component.onCompleted: {
    RegisterCommand("General", "Execute Command", "Ctrl+P",
                    () => searchUserCommandDialog.display());
    RegisterCommand("General", "Close Project", "Ctrl+W",
                    () => {
                      projectContext.CloseProject();
                      currentView.display("SelectProject.qml");
                    });
    RegisterCommand("Task", "Run Task", "Ctrl+R",
                    () => currentView.display("RunTask.qml"));
    Commit();
  }
}
