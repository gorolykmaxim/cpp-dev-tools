import QtQuick
import QtQuick.Controls
import "." as Cdt
import cdt

Cdt.Pane {
  anchors.fill: parent
  focus: true
  color: Theme.colorBgDark
  SqliteQueryEditorController {
    id: controller
  }
  SplitView {
    id: root
    anchors.fill: parent
    handle: Cdt.SplitViewHandle {
      viewId: "SqliteQueryEditor"
      view: root
    }
    Cdt.TextArea {
      SplitView.minimumWidth: 300
      SplitView.fillHeight: true
      focus: true
      detectFileLinks: false
      searchable: true
      text: controller.query
      placeholderText: "Enter SQL queries here"
      innerPadding: Theme.basePadding
      formatter: controller.formatter
      onDisplayTextChanged: controller.query = displayText
      KeyNavigation.right: tableView
      onCtrlEnterPressed: controller.executeQuery(getText(), effectiveCursorPosition)
    }
    Cdt.Text {
      visible: controller.status
      text: controller.status
      color: controller.statusColor
      elide: Text.ElideRight
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      SplitView.fillWidth: true
      SplitView.fillHeight: true
    }
    Cdt.TableView {
      id: tableView
      visible: !controller.status
      SplitView.fillWidth: true
      SplitView.fillHeight: true
      model: controller.model
    }
  }
}

