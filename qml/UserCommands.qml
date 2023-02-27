import QtQuick
import cdt

UserCommandController {
  Component.onCompleted: {
    RegisterCommand("General", "Execute Command", "Ctrl+P",
                    () => searchUserCommandDialog.display());
    RegisterCommand("General", "Close Project", "Ctrl+W",
                    () => {
                      projectContext.CloseProject();
                      currentView.source = "SelectProject.qml";
                    });
    RegisterCommand("Task", "Run Task", "Ctrl+R",
                    () => currentView.source = "RunTask.qml");
    Commit();
  }
}
