import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

Cdt.Pane {
  id: root
  property bool readOnly: false
  property string text: ""
  property alias formatter: controller.formatter
  signal searchResultsChanged()
  signal selectResult(int offset, int length)
  signal replaceResults(list<int> offsets, list<int> lengths, string text)
  color: Theme.colorBgMedium
  padding: Theme.basePadding
  visible: false
  Keys.onEscapePressed: function(e) {
    if (visible) {
      close();
      e.accepted = true;
    } else {
      e.accepted = false;
    }
  }
  onTextChanged: {
    if (visible) {
      controller.search(searchTextField.displayText, text, readOnly)
    }
  }
  function display(offset, text) {
    visible = true;
    searchTextField.text = text;
    replaceTextField.text = "";
    // When searchBar open request arrives we need to focus the search bar's
    // input regardless of what else is happenning.
    searchTextField.forceActiveFocus();
    controller.goToResultWithStartAt(offset);
  }
  function close() {
    const result = visible;
    visible = false;
    searchTextField.text = "";
    replaceTextField.text = "";
    return result;
  }
  function replaceAndSearch(replaceAll) {
    controller.replaceSearchResultWith(replaceTextField.displayText, replaceAll);
    controller.search(searchTextField.displayText, root.text, true);
  }
  TextSearchController {
    id: controller
    onSearchResultsChanged: root.searchResultsChanged()
    onReplaceResults: (o, l, t) => root.replaceResults(o, l, t)
    onSelectResult: (o, l) => root.selectResult(o, l)
  }
  ColumnLayout {
    anchors.fill: parent
    RowLayout {
      Layout.fillWidth: true
      Cdt.TextField {
        id: searchTextField
        Layout.fillWidth: true
        placeholderText: "Search text"
        onDisplayTextChanged: controller.search(displayText, root.text, true)
        Keys.onReturnPressed: e => controller.goToSearchResult(!(e.modifiers & Qt.ShiftModifier))
        Keys.onEnterPressed: e => controller.goToSearchResult(!(e.modifiers & Qt.ShiftModifier))
        KeyNavigation.right: prevResultBtn
        KeyNavigation.down: root.readOnly ? null : replaceTextField
      }
      Cdt.Text {
        text: controller.searchResultsCount
      }
      Cdt.IconButton {
        id: prevResultBtn
        buttonIcon: "arrow_upward"
        enabled: !controller.searchResultsEmpty
        onClicked: controller.goToSearchResult(false)
        KeyNavigation.right: nextResultBtn
        KeyNavigation.down: root.readOnly ? null : replaceTextField
      }
      Cdt.IconButton {
        id: nextResultBtn
        buttonIcon: "arrow_downward"
        enabled: !controller.searchResultsEmpty
        onClicked: controller.goToSearchResult(true)
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
        Keys.onReturnPressed: replaceAndSearch(false)
        Keys.onEnterPressed: replaceAndSearch(false)
        KeyNavigation.right: replaceBtn
        KeyNavigation.up: searchTextField
      }
      Cdt.Button {
        id: replaceBtn
        text: "Replace"
        onClicked: replaceAndSearch(false)
        KeyNavigation.right: replaceAllBtn
        KeyNavigation.up: searchTextField
      }
      Cdt.Button {
        id: replaceAllBtn
        text: "Replace All"
        onClicked: replaceAndSearch(true)
        KeyNavigation.up: searchTextField
      }
    }
  }
}
