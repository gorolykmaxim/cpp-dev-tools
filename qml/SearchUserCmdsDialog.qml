import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
  anchors.fill: parent
  spacing: 0

  function execCommand(eventType) {
    if (!eventType && cmdList.currentItem) {
      eventType = cmdList.currentItem.itemModel.eventType;
    }
    if (!eventType) {
      return;
    }
    dialog.reject();
    core.OnAction(eventType, []);
  }

  PaneWidget {
    Layout.fillWidth: true
    padding: basePadding
    focus: true
    ListSearchWidget {
      width: parent.width
      text: dFilter || ""
      placeholderText: "Search command"
      focus: true
      list: cmdList
      changeEventType: "daFilterChanged"
      Keys.onReturnPressed: execCommand()
      Keys.onEnterPressed: execCommand()
    }
  }
  PaneWidget {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: colorBgDark
    TextListWidget {
      id: cmdList
      anchors.fill: parent
      model: dCommands
      onItemClicked: (item) => execCommand(item.eventType)
    }
  }
}
