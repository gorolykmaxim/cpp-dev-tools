import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import cdt
import "." as Cdt

ColumnLayout {
  id: root
  property bool cursorFollowEnd: false
  property bool searchable: false
  property string text: ""
  property string color: "transparent"
  property var textFormat: TextEdit.PlainText
  property real innerPadding: 0
  property var navigationUp: null
  property var navigationDown: null
  property var navigationLeft: null
  property var navigationRight: null
  spacing: 0
  onTextChanged: {
    textArea.text = text;
    if (!cursorFollowEnd) {
      return;
    }
    textArea.cursorPosition = textArea.length;
  }
  onActiveFocusChanged: {
    if (activeFocus) {
      textArea.forceActiveFocus();
    }
  }
  function openSearchBar() {
    if (!searchable) {
      return;
    }
    searchBar.visible = true;
    searchOutputTextField.forceActiveFocus();
  }
  function closeSearchBar() {
    searchBar.visible = false;
    textArea.forceActiveFocus();
  }
  Cdt.Pane {
    id: searchBar
    Layout.fillWidth: true
    color: Theme.colorBgMedium
    padding: Theme.basePadding
    visible: false
    Keys.onEscapePressed: closeSearchBar()
    RowLayout {
      anchors.fill: parent
      Cdt.TextField {
        id: searchOutputTextField
        Layout.fillWidth: true
        text: ""
        placeholderText: "REPLACE ME PLACEHOLDER"
        // onDisplayTextChanged: core.OnAction("vaSearchExecOutput",
        //                                     [displayText])
        KeyNavigation.up: navigationUp
        KeyNavigation.down: textArea
        KeyNavigation.left: navigationLeft
        KeyNavigation.right: searchPrevBtn
        function goToSearchResult(event) {
          if (event.modifiers & Qt.ControlModifier) {
            // core.OnAction("vaPrevExecOutputSearchResult", [])
          } else {
            // core.OnAction("vaNextExecOutputSearchResult", [])
          }
        }
        Keys.onReturnPressed: goToSearchResult(event)
        Keys.onEnterPressed: goToSearchResult(event)
      }
      Cdt.Text {
        text: "NO RESULTS"
      }
      Cdt.IconButton {
        id: searchPrevBtn
        buttonIcon: "arrow_upward"
        // enabled: !vExecOutputNoSearchResults
        // onClicked: core.OnAction("vaPrevExecOutputSearchResult", [])
        KeyNavigation.up: navigationUp
        KeyNavigation.down: textArea
        KeyNavigation.left: searchOutputTextField
        KeyNavigation.right: searchNextBtn
      }
      Cdt.IconButton {
        id: searchNextBtn
        buttonIcon: "arrow_downward"
        // enabled: !vExecOutputNoSearchResults
        // onClicked: core.OnAction("vaNextExecOutputSearchResult", [])
        KeyNavigation.up: navigationUp
        KeyNavigation.down: textArea
        KeyNavigation.left: searchPrevBtn
        KeyNavigation.right: navigationRight
      }
    }
  }
  Cdt.Pane {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: root.color
    ScrollView {
      anchors.fill: parent
      TextArea {
        id: textArea
        property var allowedKeys: new Set([
          Qt.Key_Left,
          Qt.Key_Right,
          Qt.Key_Up,
          Qt.Key_Down,
          Qt.Key_Home,
          Qt.Key_End,
          Qt.Key_Escape,
        ])
        selectByMouse: true
        selectionColor: Theme.colorHighlight
        color: Theme.colorText
        leftPadding: root.innerPadding
        rightPadding: root.innerPadding
        topPadding: root.innerPadding
        bottomPadding: root.innerPadding
        textFormat: root.textFormat
        background: Rectangle {
          color: "transparent"
        }
        wrapMode: TextArea.WordWrap
        // Make text area effectively readOnly but don't hide the cursor and
        // allow navigating it using the cursor
        Keys.onPressed: event => {
          if (!allowedKeys.has(event.key) &&
              !event.matches(StandardKey.Copy) &&
              !event.matches(StandardKey.SelectAll)) {
            event.accepted = true;
          }
          if (event.key === Qt.Key_Escape) {
            closeSearchBar();
          }
        }
        onPressed: event => {
          if (event.button == Qt.RightButton) {
            contextMenu.open();
          }
        }
        KeyNavigation.up: searchBar.visible ? searchOutputTextField : navigationUp
        KeyNavigation.down: navigationDown
        KeyNavigation.left: navigationLeft
        KeyNavigation.right: navigationRight
        Menu {
          id: contextMenu
          MenuItem {
            text: "Copy"
            shortcut: StandardKey.Copy
            onTriggered: textArea.copy()
          }
          MenuSeparator {}
          MenuItem {
            text: "Find"
            enabled: searchable
            shortcut: StandardKey.Find
            onTriggered: openSearchBar()
          }
          MenuSeparator {}
          MenuItem {
            text: "Select All"
            shortcut: StandardKey.SelectAll
            onTriggered: textArea.selectAll()
          }
        }
      }
    }
  }
}
