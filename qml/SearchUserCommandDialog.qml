import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import cdt
import "." as Cdt

Dialog {
  property var userCommands: null
  id: dialog
  Component.onCompleted: input.forceActiveFocus()
  onVisibleChanged: {
    if (visible) {
      controller.userCommands.filter = "";
      controller.LoadUserCommands(userCommands);
    }
  }
  function executeTask() {
    cmdList.ifCurrentItem('callback', (cb) => {
      dialog.accept();
      cb();
    });
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
  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 1
    spacing: 0
    Cdt.Pane {
      Layout.fillWidth: true
      padding: Theme.basePadding
      Cdt.ListSearch {
        id: input
        width: parent.width
        text: controller.userCommands.filter
        placeholderText: "Search command"
        list: cmdList
        onDisplayTextChanged: controller.userCommands.filter = displayText
        onEnterPressed: executeTask()
      }
    }
    Cdt.Pane {
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: Theme.colorBgDark
      Cdt.TextList {
        id: cmdList
        anchors.fill: parent
        model: controller.userCommands
        onItemLeftClicked: executeTask()
      }
    }
  }
}
