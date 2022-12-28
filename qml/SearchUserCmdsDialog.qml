import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
  anchors.fill: parent
  spacing: 0

  function execCommand(eventType) {
    dialog.reject();
    core.OnAction(eventType, []);
  }

  PaneWidget {
    Layout.fillWidth: true
    padding: basePadding
    focus: true
    TextFieldWidget {
      width: parent.width
      text: dFilter || ""
      placeholderText: "Search command"
      focus: true
      onDisplayTextChanged: core.OnAction("daFilterChanged", [displayText])
      Keys.onReturnPressed: execCommand(list.currentItem.itemModel.eventType)
      Keys.onEnterPressed: execCommand(list.currentItem.itemModel.eventType)
      Keys.onDownPressed: list.incrementCurrentIndex()
      Keys.onUpPressed: list.decrementCurrentIndex()
    }
  }
  PaneWidget {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: colorBgDark
    TextListWidget {
      id: list
      anchors.fill: parent
      model: dCommands
      onItemClicked: (item) => execCommand(item.eventType)
    }
  }
}
