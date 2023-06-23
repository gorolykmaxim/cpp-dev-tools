import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

Cdt.Pane {
  id: root
  property bool readOnly: false
  property alias searchTerm: searchTextField.displayText
  property alias replacementTerm: replaceTextField.displayText
  property alias searchResultCount: searchResultCountText.text
  property bool noSearchResults: true
  signal search()
  signal replace(bool replaceAll)
  signal goToSearchResult(bool next)
  function display(term) {
    visible = true;
    searchTextField.text = term;
    replaceTextField.text = "";
    // When searchBar open request arrives we need to focus the search bar's
    // input regardless of what else is happenning.
    searchTextField.forceActiveFocus();
  }
  function close() {
    const result = visible;
    visible = false;
    searchTextField.text = "";
    replaceTextField.text = "";
    return result;
  }
  visible: false
  padding: Theme.basePadding
  Keys.onEscapePressed: function(e) {
    if (visible) {
      close();
      e.accepted = true;
    } else {
      e.accepted = false;
    }
  }
  ColumnLayout {
    anchors.fill: parent
    RowLayout {
      Layout.fillWidth: true
      Cdt.TextField {
        id: searchTextField
        Layout.fillWidth: true
        placeholderText: "Search text"
        onDisplayTextChanged: search()
        Keys.onReturnPressed: e => goToSearchResult(!(e.modifiers & Qt.ShiftModifier))
        Keys.onEnterPressed: e => goToSearchResult(!(e.modifiers & Qt.ShiftModifier))
        KeyNavigation.right: prevResultBtn
        KeyNavigation.down: root.readOnly ? null : replaceTextField
      }
      Cdt.Text {
        id: searchResultCountText
      }
      Cdt.IconButton {
        id: prevResultBtn
        buttonIcon: "arrow_upward"
        enabled: !noSearchResults
        onClicked: goToSearchResult(false)
        KeyNavigation.right: nextResultBtn
        KeyNavigation.down: root.readOnly ? null : replaceTextField
      }
      Cdt.IconButton {
        id: nextResultBtn
        buttonIcon: "arrow_downward"
        enabled: !noSearchResults
        onClicked: goToSearchResult(true)
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
        Keys.onReturnPressed: replace(false)
        Keys.onEnterPressed: replace(false)
        KeyNavigation.right: replaceBtn
        KeyNavigation.up: searchTextField
      }
      Cdt.Button {
        id: replaceBtn
        text: "Replace"
        onClicked: replace(false)
        KeyNavigation.right: replaceAllBtn
        KeyNavigation.up: searchTextField
      }
      Cdt.Button {
        id: replaceAllBtn
        text: "Replace All"
        onClicked: replace(true)
        KeyNavigation.up: searchTextField
      }
    }
  }
}
