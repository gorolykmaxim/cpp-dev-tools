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
  SplitView {
    id: sidebar
    SplitView.minimumWidth: 300
    SplitView.fillHeight: true
    orientation: Qt.Vertical
    handle: Cdt.SplitViewHandle {
      viewId: "GitCommitSidebar"
      view: sidebar
    }
    Cdt.SearchableTextList {
      SplitView.fillWidth: true
      SplitView.fillHeight: true
      searchPlaceholderText: "Search changed file"
      searchableModel: controller.files
    }
    ColumnLayout {
      SplitView.fillWidth: true
      SplitView.minimumHeight: 300
      spacing: 0
      Cdt.TextArea {
        Layout.fillWidth: true
        Layout.fillHeight: true
        placeholderText: "Commit Message"
        focus: true
        color: Theme.colorBgDark
        innerPadding: Theme.basePadding
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
          }
          Cdt.Button {
            text: "Commit All"
          }
          Cdt.CheckBox {
            text: "Amend"
            Layout.fillWidth: true
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
    }
  }
}
