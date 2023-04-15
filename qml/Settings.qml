import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "." as Cdt
import cdt

Cdt.Pane {
  anchors.fill: parent
  focus: true
  color: Theme.colorBgMedium
  padding: Theme.basePadding
  SettingsController {
    id: controller
  }
  ColumnLayout {
    id: root
    anchors.fill: parent
    ScrollView {
      focus: true
      Layout.fillWidth: true
      Layout.fillHeight: true
      GridLayout {
        width: root.width
        columns: 2
        Cdt.Text {
          text: "Open In Editor Command"
          Layout.minimumWidth: 200
        }
        Cdt.TextField {
          text: controller.openInEditorCommand
          onDisplayTextChanged: controller.openInEditorCommand = displayText
          focus: true
          Layout.fillWidth: true
          KeyNavigation.down: saveBtn
        }
      }
    }
    Cdt.Button {
      id: saveBtn
      Layout.alignment: Qt.AlignRight
      text: "Save"
      onClicked: controller.save()
    }
  }
}
