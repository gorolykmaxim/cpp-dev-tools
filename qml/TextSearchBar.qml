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
  color: Theme.colorBgMedium
  padding: Theme.basePadding
  visible: false
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
    return result;
  }
  TextSearchController {
    id: controller
    onSearchResultsChanged: root.searchResultsChanged()
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
      function replaceAndSearch(replaceAll) {
        controller.replaceSearchResultWith(replaceOutputTextField.displayText, replaceAll);
        controller.search(searchOutputTextField.displayText, root.text, true);
      }
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
