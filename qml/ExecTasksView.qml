import QtQuick
import QtQuick.Layouts
import "Common.js" as Cmn

PaneWidget {
  anchors.fill: parent
  color: colorBgDark
  focus: true
  RowLayout {
    anchors.fill: parent
    spacing: 0
    ColumnLayout {
      Layout.maximumWidth: 300
      Layout.minimumWidth: 300
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
                                                 "taskName")
          Keys.onEnterPressed: Cmn.onListAction(taskList, "vaExecuteTask",
                                                "taskName")
          Keys.onDownPressed: taskList.incrementCurrentIndex()
          Keys.onUpPressed: taskList.decrementCurrentIndex()
        }
      }
      TextListWidget {
        id: taskList
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: vTasks
        onItemClicked: (item) => core.OnAction("vaExecuteTask", [item.taskName])
      }
    }
    ColumnLayout {
      Layout.fillWidth: true
      spacing: 0
      PaneWidget {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: colorBgMedium
        padding: basePadding
        RowLayout {
          IconWidget {
            icon: vExecIcon || ""
            color: vExecIconColor || colorText
          }
          Column {
            Layout.fillWidth: true
            TextWidget {
              text: vExecName || ""
            }
            TextWidget {
              text: vExecCmd || ""
              color: colorSubText
            }
          }
        }
      }
    }
    ColumnLayout {
      Layout.maximumWidth: 300
      Layout.minimumWidth: 300
      spacing: 0
      PaneWidget {
        Layout.fillWidth: true
        color: colorBgMedium
        padding: basePadding
        TextFieldWidget {
          width: parent.width
          text: vExecFilter || ""
          placeholderText: "Search execution"
          onDisplayTextChanged: core.OnAction("vaExecFilterChanged",
                                              [displayText])
          Keys.onDownPressed: execList.incrementCurrentIndex()
          Keys.onUpPressed: execList.decrementCurrentIndex()
        }
      }
      TextListWidget {
        id: execList
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: vExecs
        onCurrentIndexChanged: {
          core.OnAction("vaExecSelected", [execList.currentItem.itemModel.id]);
        }
      }
    }
  }
}
