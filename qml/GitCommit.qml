import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "." as Cdt
import cdt

SplitView {
  id: root
  anchors.fill: parent
  handle: Cdt.SplitViewHandle {
    viewId: "GitCommitWindow"
    view: root
  }
  GitCommitController {
    id: controller
  }
  Keys.onPressed: function(e) {
    if (e.key === Qt.Key_F5) {
      controller.findChangedFiles()
    }
  }
  SplitView {
    id: sidebar
    SplitView.minimumWidth: 300
    SplitView.fillHeight: true
    orientation: Qt.Vertical
    handle: Cdt.SplitViewHandle {
      viewId: "GitCommitSidebar"
      view: sidebar
    }
    ColumnLayout {
      SplitView.fillWidth: true
      SplitView.fillHeight: true
      spacing: 0
      Cdt.Pane {
        Layout.fillWidth: true
        color: Theme.colorBgMedium
        focus: !controller.hasChanges
        RowLayout {
          anchors.fill: parent
          Cdt.Text {
            text: "Changed Files"
            Layout.fillWidth: true
            padding: Theme.basePadding
          }
          Cdt.IconButton {
            buttonIcon: "loop"
            onClicked: controller.findChangedFiles()
            focus: true
          }
        }
      }
      Cdt.SearchableTextList {
        Layout.fillWidth: true
        Layout.fillHeight: true
        searchPlaceholderText: "Search changed file"
        searchableModel: controller.files
        enabled: controller.hasChanges
      }
    }
    ColumnLayout {
      SplitView.fillWidth: true
      SplitView.minimumHeight: 300
      spacing: 0
      Cdt.TextArea {
        Layout.fillWidth: true
        Layout.fillHeight: true
        placeholderText: "Commit Message"
        focus: controller.hasChanges
        color: Theme.colorBgDark
        innerPadding: Theme.basePadding
        enabled: controller.hasChanges
      }
      Cdt.Pane {
        Layout.fillWidth: true
        color: Theme.colorBgMedium
        padding: Theme.basePadding
        RowLayout {
          anchors.fill: parent
          spacing: Theme.basePadding
          Cdt.Button {
            text: "Commit"
            enabled: controller.hasChanges
          }
          Cdt.Button {
            text: "Commit All"
            enabled: controller.hasChanges
          }
          Cdt.CheckBox {
            text: "Amend"
            Layout.fillWidth: true
            enabled: controller.hasChanges
          }
        }
      }
    }
  }
  ColumnLayout {
    SplitView.fillWidth: true
    SplitView.fillHeight: true
    spacing: 0
    Cdt.Pane {
      color: Theme.colorBgMedium
      Layout.fillWidth: true
      Cdt.Text {
        anchors.fill: parent
        text: "Path to selected file"
        elide: Text.ElideLeft
        padding: Theme.basePadding
      }
    }
    Cdt.TextArea {
      Layout.fillWidth: true
      Layout.fillHeight: true
      readonly: true
      placeholderText: "Diff will be displayed here"
      innerPadding: Theme.basePadding
      color: Theme.colorBgDark
      wrapMode: TextEdit.NoWrap
      enabled: controller.hasChanges
    }
  }
}
