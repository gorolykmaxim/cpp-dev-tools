import QtQuick
import QtQuick.Controls
import cdt
import "." as Cdt

Dialog {
  id: dialog
  Connections {
      target: viewSystem
      function onSearchUserCommandDialogDisplayed() {
        controller.loadUserCommands();
        visible = true;
      }
      function onDialogClosed() {
        dialog.reject();
      }
  }
  width: Theme.centeredViewWidth
  height: parent.height * 0.8
  padding: 0
  modal: true
  visible: false
  anchors.centerIn: parent
  background: Rectangle {
    color: Theme.colorBackground
    radius: Theme.baseRadius
    border.width: 1
    border.color: Theme.colorBorder
  }
  SearchUserCommandController {
    id: controller
    onCommandExecuted: dialog.accept()
  }
  Cdt.SearchableTextList {
    id: textList
    anchors.fill: parent
    anchors.margins: 1
    searchPlaceholderText: "Search command"
    focus: true
    searchableModel: controller.userCommands
    onItemSelected: controller.executeSelectedCommand()
  }
}
