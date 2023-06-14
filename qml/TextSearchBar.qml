import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

Cdt.Pane {
  id: root
  property bool readOnly: false
  color: Theme.colorBgMedium
  padding: Theme.basePadding
  visible: false
  function display() {
    visible = true;
    // When searchBar open request arrives we need to focus the search bar's
    // input regardless of what else is happenning.
    searchTextField.text = "";
    replaceTextField.text = "";
    searchTextField.forceActiveFocus();
  }
  function close() {
    const result = visible;
    visible = false;
    return result;
  }
  ColumnLayout {
    anchors.fill: parent
    RowLayout {
      Layout.fillWidth: true
      Cdt.TextField {
        id: searchTextField
        Layout.fillWidth: true
        placeholderText: "Search text"
        onDisplayTextChanged: console.log('search', displayText, true)
        Keys.onReturnPressed: e => console.log('go to search result', !(e.modifiers & Qt.ShiftModifier))
        Keys.onEnterPressed: e => console.log('go to search result', !(e.modifiers & Qt.ShiftModifier))
        KeyNavigation.right: prevResultBtn
        KeyNavigation.down: root.readOnly ? null : replaceTextField
      }
      Cdt.Text {
        text: "Results count"
      }
      Cdt.IconButton {
        id: prevResultBtn
        buttonIcon: "arrow_upward"
        onClicked: console.log('go to search result', false)
        KeyNavigation.right: nextResultBtn
        KeyNavigation.down: root.readOnly ? null : replaceTextField
      }
      Cdt.IconButton {
        id: nextResultBtn
        buttonIcon: "arrow_downward"
        onClicked: console.log('go to search result', true)
        KeyNavigation.down: root.readOnly ? null : replaceTextField
      }
    }
    RowLayout {
      visible: !root.readOnly
      Layout.fillWidth: true
      Cdt.TextField {
        id: replaceTextField
        Layout.fillWidth: true
        placeholderText: "Replace text"
        Keys.onReturnPressed: console.log('replace and search', false)
        Keys.onEnterPressed: console.log('replace and search', false)
        KeyNavigation.right: replaceBtn
        KeyNavigation.up: searchTextField
      }
      Cdt.Button {
        id: replaceBtn
        text: "Replace"
        onClicked: console.log('replace and search', false)
        KeyNavigation.right: replaceAllBtn
        KeyNavigation.up: searchTextField
      }
      Cdt.Button {
        id: replaceAllBtn
        text: "Replace All"
        onClicked: console.log('replace and search', true)
        KeyNavigation.up: searchTextField
      }
    }
  }
}
