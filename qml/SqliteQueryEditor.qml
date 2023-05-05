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
    ScrollView {
      SplitView.minimumWidth: 300
      SplitView.fillHeight: true
      focus: true
      TextArea {
        textFormat: TextEdit.PlainText
        color: Theme.colorText
        focus: true
        placeholderText: "Enter SQL queries here"
        placeholderTextColor: Theme.colorSubText
        wrapMode: TextArea.WordWrap
        KeyNavigation.right: tableView
        Keys.onPressed: function (e) {
          if ((e.key === Qt.Key_Enter || e.key === Qt.Key_Return) && (e.modifiers & Qt.ControlModifier)) {
            controller.executeQuery(getText(0, length), cursorPosition)
          }
        }
      }
    }
    Cdt.Text {
      visible: controller.status
      text: controller.status
      color: controller.statusColor
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

