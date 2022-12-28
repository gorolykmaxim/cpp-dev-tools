import QtQuick
import QtQuick.Layouts
import "Common.js" as Cmn

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
          placeholderText: "Search task"
          focus: true
          onDisplayTextChanged: core.OnAction("vaTaskFilterChanged",
                                              [displayText])
          Keys.onReturnPressed: Cmn.onListAction(taskList, "vaExecuteTask",
                                                 "title")
          Keys.onEnterPressed: Cmn.onListAction(taskList, "vaExecuteTask",
                                                "title")
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
