import QtQuick
import QtQuick.Controls
import cdt
import "." as Cdt

Dialog {
  id: dialog
  Connections {
      target: viewSystem
      function onSearchUserCommandDialogDisplayed() {
        controller.LoadUserCommands();
        visible = true;
        textList.forceActiveFocus()
      }
      function onDialogClosed() {
        dialog.reject();
      }
  }
  width: 500
  height: parent.height * 0.8
  padding: 0
  modal: true
  visible: false
  anchors.centerIn: parent
  background: Rectangle {
    color: Theme.colorBgMedium
    radius: Theme.baseRadius
    border.width: 1
    border.color: Theme.colorBgLight
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
    searchableModel: controller.userCommands
    onItemSelected: ifCurrentItem('index', controller.ExecuteCommand)
  }
}
