import QtQuick
import QtQuick.Controls
import cdt
import "." as Cdt

Dialog {
  property var userCommands: null
  id: dialog
  Component.onCompleted: textList.forceActiveFocus()
  onVisibleChanged: {
    if (visible) {
      controller.userCommands.filter = "";
      controller.LoadUserCommands(userCommands);
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
  }
  Cdt.SearchableTextList {
    id: textList
    anchors.fill: parent
    anchors.margins: 1
    placeholderText: "Search command"
    searchableModel: controller.userCommands
    onItemSelected: ifCurrentItem('callback', (cb) => {
      dialog.accept();
      cb();
    })
  }
}
