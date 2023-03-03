import QtQuick
import cdt

UserCommandController {
  Component.onCompleted: {
    RegisterCommand("General", "Execute Command", "Ctrl+P",
                    () => viewController.DisplaySearchUserCommandDialog());
    RegisterCommand("General", "Close Project", "Ctrl+W",
                    () => {
                      projectContext.CloseProject();
                      viewController.currentView = "SelectProject.qml";
                    });
    RegisterCommand("Task", "Run Task", "Ctrl+R",
                    () => viewController.currentView = "RunTask.qml");
    Commit();
  }
}
