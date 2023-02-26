import QtQuick
import cdt

UserCommandController {
  Component.onCompleted: {
    RegisterCommand("Group 1", "Task 1", "Ctrl+1",
                    () => console.log("COMMAND 1"));
    RegisterCommand("Group 2", "Task 2", "Ctrl+2",
                    () => console.log("COMMAND 2"));
    Commit();
  }
}
