import QtQuick
import QtQuick.Layouts

PaneWidget {
  anchors.fill: parent
  color: colorBgDark
  focus: true
  RowLayout {
    anchors.fill: parent
    ColumnLayout {
      Layout.maximumWidth: 300
      spacing: 0
      PaneWidget {
        Layout.fillWidth: true
        focus: true
        color: colorBgMedium
        padding: basePadding
        TextFieldWidget {
          width: parent.width
          text: vTaskFilter || ""
          placeholderText: "Search task by name"
          focus: true
          onDisplayTextChanged: core.OnAction("vaTaskFilterChanged",
                                              [displayText])
          Keys.onReturnPressed: core.OnAction(
              "vaExecuteTask", [taskList.currentItem.itemModel.title])
          Keys.onEnterPressed: core.OnAction(
              "vaExecuteTask", [taskList.currentItem.itemModel.title])
          Keys.onDownPressed: taskList.incrementCurrentIndex()
          Keys.onUpPressed: taskList.decrementCurrentIndex()
        }
      }
      TextListWidget {
        id: taskList
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: vTasks
        onItemClicked: (item) => core.OnAction("vaExecuteTask", [item.title])
      }
    }
  }
}
