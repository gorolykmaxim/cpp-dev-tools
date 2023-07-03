import QtQuick
import QtQuick.Layouts
import Qt.labs.platform
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
      enabled: shortcutModel.selectedShortcut
      onDisplayTextChanged: shortcutModel.setSelectedShortcut(displayText)
      placeholderText: "Shortcut"
      Layout.fillWidth: true
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
    onItemSelected: {
      shortcutModel.selectCurrentCommand();
      shortcutTextField.forceActiveFocus();
    }
    onItemRightClicked: contextMenu.open()
    KeyNavigation.up: shortcutTextField.enabled ? shortcutTextField : backBtn
  }
  Menu {
    id: contextMenu
    MenuItem {
      text: "Reset Change"
      shortcut: gSC("KeyboardShortcuts", "Reset Changed")
      enabled: list.activeFocus
      onTriggered: shortcutModel.resetCurrentShortcut()
    }
    MenuItem {
      text: "Reset All Changes"
      shortcut: gSC("KeyboardShortcuts", "Reset All Changes")
      enabled: list.activeFocus
      onTriggered: shortcutModel.resetAllShortcuts()
    }
    MenuItem {
      text: "Restore Default"
      shortcut: gSC("KeyboardShortcuts", "Restore Default")
      enabled: list.activeFocus
      onTriggered: shortcutModel.restoreDefaultOfCurrentShortcut()
    }
    MenuItem {
      text: "Restore All Defaults"
      shortcut: gSC("KeyboardShortcuts", "Restore All Defaults")
      enabled: list.activeFocus
      onTriggered: shortcutModel.restoreDefaultOfAllShortcuts()
    }
  }
}
