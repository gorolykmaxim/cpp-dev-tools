import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

ColumnLayout {
  required property QtObject shortcutModel
  signal back()
  anchors.fill: parent
  spacing: 0
  Component.onCompleted: shortcutModel.load()
  Keys.onEscapePressed: back()
  RowLayout {
    Layout.fillWidth: true
    Layout.margins: Theme.basePadding
    spacing: Theme.basePadding
    Cdt.Text {
      text: shortcutModel.selectedCommand
    }
    Cdt.TextField {
      id: shortcutTextField
      text: shortcutModel.selectedShortcut
      placeholderText: "Shortcut"
      Layout.fillWidth: true
      KeyNavigation.right: resetBtn
      KeyNavigation.down: list
    }
    Cdt.Button {
      id: resetBtn
      text: "Reset Shortcut"
      KeyNavigation.right: backBtn
      KeyNavigation.down: list
    }
    Cdt.Button {
      id: backBtn
      text: "Back"
      onClicked: back()
      KeyNavigation.down: list
    }
  }
  Cdt.SearchableTextList {
    id: list
    Layout.fillWidth: true
    Layout.fillHeight: true
    searchPlaceholderText: "Search shortcut"
    searchableModel: shortcutModel
    focus: true
    onItemSelected: shortcutModel.selectCurrentCommand()
    KeyNavigation.up: shortcutTextField
  }
}
