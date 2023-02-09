import QtQuick
import QtQuick.Layouts
import Qt.labs.platform
import "Common.js" as Cmn

ColumnLayout {
  anchors.fill: parent
  spacing: 0
  TextWidget {
    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
    visible: vLoading || vTasksEmpty
    text: vLoading ? "Looking for tasks..." : "No tasks found"
  }
  PaneWidget {
    Layout.fillWidth: true
    color: colorBgMedium
    focus: true
    padding: basePadding
    visible: !vLoading && !vTasksEmpty
    ListSearchWidget {
      id: input
      text: vFilter || ""
      placeholderText: "Search task"
      focus: true
      anchors.fill: parent
      list: taskList
      changeEventType: "vaFilterChanged"
      enterEventType: "vaTaskChosen"
      listIdFieldName: "idx"
    }
  }
  PaneWidget {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: colorBgDark
    visible: !vLoading && !vTasksEmpty
    TextListWidget {
      id: taskList
      anchors.fill: parent
      model: vTasks
      elide: Text.ElideLeft
      onItemLeftClicked: Cmn.onListAction(taskList, "vaTaskChosen", "idx")
    }
  }
}
