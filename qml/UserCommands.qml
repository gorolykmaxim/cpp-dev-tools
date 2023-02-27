import QtQuick
import cdt

UserCommandController {
  Component.onCompleted: {
    RegisterCommand("General", "Execute Command", "Ctrl+P",
                    () => searchUserCommandDialog.visible = true);
    Commit();
  }
}
