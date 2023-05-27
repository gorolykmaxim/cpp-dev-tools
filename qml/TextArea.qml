import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import Qt.labs.platform
import cdt
import "." as Cdt
import "Common.js" as Common

FocusScope {
  id: root
  property bool cursorFollowEnd: false
  property bool searchable: false
  property bool readonly: false
  property bool isLoading: false
  property string text: ""
  property string color: "transparent"
  property real innerPadding: 0
  property alias formatter: controller.formatter
  property bool detectFileLinks: true
  property int wrapMode: Controls.TextArea.WordWrap
  property alias placeholderText: textArea.placeholderText
  property alias effectiveCursorPosition: textArea.cursorPosition
  property alias displayText: textArea.text
  property int cursorPosition: -1
  signal ctrlEnterPressed()
  onCursorPositionChanged: textArea.cursorPosition = root.cursorPosition
  onTextChanged: {
    controller.resetCursorPositionHistory();
    if (textArea.text && text.startsWith(textArea.text)) {
      setText(text);
    } else {
      isLoading = true;
      // Setting textArea's text to "" is necessary because if we are currently
      // displaying a large text and the ScrollView's position is outside of the
      // future ScrollView's content region - both ScrollView and TextArea won't
      // get re-rendered right away and instead we will see nothing.
      // By setting text to "" we trigger that bug first and after a delay we set
      // text to the actual next value - which will make TextArea and ScrollView
      // re-render with the new text value properly.
      textArea.text = "";
      textSetDelay.restart();
    }
  }
  function setText(newText) {
    controller.findFileLinks(newText);
    textArea.text = newText;
    isLoading = false;
  }
  function openSearchBar() {
    if (!searchable) {
      return;
    }
    const position = textArea.selectionStart;
    searchOutputTextField.text = textArea.selectedText;
    replaceOutputTextField.text = "";
    searchBar.displaySearchBar = true;
    // When searchBar open request arrives we need to focus the search bar's
    // input regardless of what else is happenning.
    searchOutputTextField.forceActiveFocus();
    controller.goToResultWithStartAt(position);
  }
  function closeSearchBar() {
    searchBar.displaySearchBar = false;
  }
  function rehighlightBlockByLineNumber(i) {
    controller.rehighlightBlockByLineNumber(i);
  }
  function getText() {
    return textArea.getText(0, textArea.length)
  }
  function replaceAndSearch(replaceAll) {
    controller.replaceSearchResultWith(replaceOutputTextField.displayText, replaceAll);
    controller.search(searchOutputTextField.displayText, root.getText(), true);
  }
  Timer {
    id: textSetDelay
    interval: 1 // Having it as just 0 does not always trigger "Loading"
    onTriggered: setText(text)
  }
  TextAreaController {
    id: controller
    document: textArea.textDocument
    detectFileLinks: root.detectFileLinks && root.readonly
    onSelectText: function(start, end) {
      textArea.ignoreTextChange = true;
      textArea.select(start, end);
      textArea.ignoreTextChange = false;
    }
    onReplaceText: function(start, end, newText) {
      textArea.ignoreTextChange = true;
      textArea.remove(start, end);
      textArea.insert(start, newText);
      textArea.ignoreTextChange = false;
    }
    onChangeCursorPosition: pos => textArea.cursorPosition = pos
  }
  Cdt.Pane {
    visible: isLoading
    anchors.fill: parent
    color: root.color
    Cdt.Text {
      anchors.fill: parent
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      text: "Loading..."
      elide: Text.ElideRight
    }
  }
  ColumnLayout {
    visible: !isLoading
    anchors.fill: parent
    spacing: 0
    Cdt.Pane {
      id: searchBar
      property bool displaySearchBar: false
      Layout.fillWidth: true
      color: Theme.colorBgMedium
      padding: Theme.basePadding
      visible: displaySearchBar
      Keys.onEscapePressed: closeSearchBar(true)
      ColumnLayout {
        anchors.fill: parent
        RowLayout {
          Layout.fillWidth: true
          Cdt.TextField {
            id: searchOutputTextField
            Layout.fillWidth: true
            placeholderText: "Search text"
            onDisplayTextChanged: controller.search(displayText, root.getText(), true)
            // Don't attach navigation down to TextArea if searchbar is invisible, because in case there
            // is a component above our Cdt.TextArea - we won't be able to navigation to it via "up",
            // because "up" of our internal TextArea is searchbar and it is invisible (thus can't be reached).
            KeyNavigation.down: searchBar.displaySearchBar ? (root.readonly ? textArea : replaceOutputTextField) : null
            KeyNavigation.right: searchPrevBtn
            Keys.onReturnPressed: e => controller.goToSearchResult(!(e.modifiers & Qt.ShiftModifier))
            Keys.onEnterPressed: e => controller.goToSearchResult(!(e.modifiers & Qt.ShiftModifier))
          }
          Cdt.Text {
            text: controller.searchResultsCount
          }
          Cdt.IconButton {
            id: searchPrevBtn
            buttonIcon: "arrow_upward"
            enabled: !controller.searchResultsEmpty
            onClicked: controller.goToSearchResult(false)
            KeyNavigation.down: searchBar.displaySearchBar ? (root.readonly ? textArea : replaceAllBtn) : null
            KeyNavigation.right: searchNextBtn
          }
          Cdt.IconButton {
            id: searchNextBtn
            buttonIcon: "arrow_downward"
            enabled: !controller.searchResultsEmpty
            onClicked: controller.goToSearchResult(true)
            KeyNavigation.down: searchBar.displaySearchBar ? (root.readonly ? textArea : replaceAllBtn) : null
          }
        }
        RowLayout {
          visible: !root.readonly
          Layout.fillWidth: true
          Cdt.TextField {
            id: replaceOutputTextField
            Layout.fillWidth: true
            placeholderText: "Replace text"
            KeyNavigation.down: searchBar.displaySearchBar ? textArea : null
            KeyNavigation.right: replaceBtn
            Keys.onReturnPressed: replaceAndSearch(false)
            Keys.onEnterPressed: replaceAndSearch(false)
          }
          Cdt.Button {
            id: replaceBtn
            text: "Replace"
            onClicked: replaceAndSearch(false)
            KeyNavigation.up: searchOutputTextField
            KeyNavigation.right: replaceAllBtn
            KeyNavigation.down: searchBar.displaySearchBar ? textArea : null
          }
          Cdt.Button {
            id: replaceAllBtn
            text: "Replace All"
            onClicked: replaceAndSearch(true)
            KeyNavigation.down: searchBar.displaySearchBar ? textArea : null
          }
        }
      }
    }
    Cdt.Pane {
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: root.color
      focus: !searchBar.displaySearchBar
      Controls.ScrollView {
        anchors.fill: parent
        focus: true
        Keys.forwardTo: [root]
        Controls.TextArea {
          id: textArea
          property var allowedReadonlyKeys: new Set([
            Qt.Key_Left,
            Qt.Key_Right,
            Qt.Key_Up,
            Qt.Key_Down,
            Qt.Key_Home,
            Qt.Key_End,
            Qt.Key_Escape,
          ])
          property bool ignoreTextChange: false
          selectByMouse: true
          enabled: root.enabled
          selectionColor: Theme.colorHighlight
          color: enabled ? Theme.colorText : Theme.colorSubText
          leftPadding: root.innerPadding
          rightPadding: root.innerPadding
          topPadding: root.innerPadding
          bottomPadding: root.innerPadding
          textFormat: TextEdit.PlainText
          placeholderTextColor: Theme.colorSubText
          font.family: monoFontFamily
          font.pointSize: monoFontSize
          focus: true
          background: Rectangle {
            color: "transparent"
          }
          wrapMode: root.wrapMode
          onCursorPositionChanged: controller.saveCursorPosition(textArea.cursorPosition)
          onTextChanged: function () {
            if (ignoreTextChange) {
              return;
            }
            if (root.cursorFollowEnd) {
              textArea.cursorPosition = length - 1;
            } else if (root.cursorPosition >= 0) {
              textArea.cursorPosition = root.cursorPosition;
            }
            if (searchBar.displaySearchBar) {
              controller.search(searchOutputTextField.displayText, root.getText(), root.readonly)
            }
          }
          Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Escape) {
              event.accepted = true;
              if (searchBar.displaySearchBar) {
                closeSearchBar(true);
              } else if (textArea.selectedText) {
                textArea.deselect();
              } else {
                event.accepted = false;
              }
            } else if (event.key === Qt.Key_PageUp) {
              controller.goToPage(root.getText(), true);
            } else if (event.key === Qt.Key_PageDown) {
              controller.goToPage(root.getText(), false);
            } else if ((event.key === Qt.Key_Enter || event.key === Qt.Key_Return) && (event.modifiers & Qt.ControlModifier)) {
              root.ctrlEnterPressed()
            }
            if (root.readonly && !allowedReadonlyKeys.has(event.key) &&
                !event.matches(StandardKey.Copy) &&
                !event.matches(StandardKey.SelectAll)) {
              event.accepted = true;
            }
          }
          onPressed: e => Common.handleRightClick(textArea, contextMenu, e)
          KeyNavigation.up: searchBar.displaySearchBar ? (root.readonly ? searchOutputTextField : replaceOutputTextField) : null
          Menu {
            id: contextMenu
            MenuItem {
              text: "Cut"
              shortcut: "Ctrl+X"
              enabled: textArea.activeFocus && !root.readonly
              onTriggered: textArea.cut()
            }
            MenuItem {
              text: "Copy"
              shortcut: "Ctrl+C"
              enabled: textArea.activeFocus
              onTriggered: textArea.copy()
            }
            MenuItem {
              text: "Paste"
              shortcut: "Ctrl+V"
              enabled: textArea.activeFocus && !root.readonly
              onTriggered: textArea.paste()
            }
            MenuSeparator {}
            MenuItem {
              text: "Find"
              enabled: textArea.activeFocus && searchable
              shortcut: "Ctrl+F"
              onTriggered: openSearchBar()
            }
            MenuSeparator {}
            MenuItem {
              text: "Select All"
              enabled: textArea.activeFocus
              shortcut: "Ctrl+A"
              onTriggered: textArea.selectAll()
            }
            MenuItem {
              text: "Select Word"
              enabled: textArea.activeFocus
              shortcut: "Ctrl+D"
              onTriggered: textArea.selectWord()
            }
            MenuItem {
              text: "Previous Cursor Position"
              enabled: textArea.activeFocus
              shortcut: "Ctrl+Alt+Left"
              onTriggered: controller.goToCursorPosition(false);
            }
            MenuItem {
              text: "Next Cursor Position"
              enabled: textArea.activeFocus
              shortcut: "Ctrl+Alt+Right"
              onTriggered: controller.goToCursorPosition(true)
            }
            MenuItem {
              text: "Open File In Editor"
              enabled: controller.detectFileLinks && textArea.activeFocus && controller.isCursorOnLink
              shortcut: "Ctrl+O"
              onTriggered: controller.openFileLinkAtCursor()
            }
            MenuItem {
              text: "Previous File Link"
              enabled: controller.detectFileLinks && textArea.activeFocus
              shortcut: "Ctrl+Alt+Up"
              onTriggered: controller.goToFileLink(false)
            }
            MenuItem {
              text: "Next File Link"
              enabled: controller.detectFileLinks && textArea.activeFocus
              shortcut: "Ctrl+Alt+Down"
              onTriggered: controller.goToFileLink(true)
            }
          }
        }
      }
    }
  }
}
