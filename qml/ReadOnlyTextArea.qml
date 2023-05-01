import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import cdt
import "." as Cdt
import "Common.js" as Common

FocusScope {
  id: root
  property bool cursorFollowEnd: false
  property bool searchable: false
  property bool isLoading: false
  property string text: ""
  property string color: "transparent"
  property real innerPadding: 0
  property alias formatter: controller.formatter
  property alias detectFileLinks: controller.detectFileLinks
  property int cursorPosition: -1
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
    if (cursorFollowEnd) {
      textArea.cursorPosition = textArea.length - 1;
    } else if (root.cursorPosition >= 0) {
      textArea.cursorPosition = root.cursorPosition;
    }
    if (searchBar.visible) {
      controller.search(searchOutputTextField.displayText, root.getText())
    }
  }
  function openSearchBar() {
    if (!searchable) {
      return;
    }
    const position = textArea.selectionStart;
    searchOutputTextField.text = textArea.selectedText;
    searchBar.visible = true;
    // When searchBar open request arrives we need to focus the search bar's
    // input regardless of what else is happenning.
    searchOutputTextField.forceActiveFocus();
    controller.goToResultWithStartAt(position);
  }
  function closeSearchBar() {
    searchBar.visible = false;
  }
  function rehighlightBlockByLineNumber(i) {
    controller.rehighlightBlockByLineNumber(i);
  }
  function getText() {
    return textArea.getText(0, textArea.length)
  }
  Timer {
    id: textSetDelay
    interval: 1 // Having it as just 0 does not always trigger "Loading"
    onTriggered: setText(text)
  }
  TextAreaController {
    id: controller
    document: textArea.textDocument
    onSelectText: (start, end) => textArea.select(start, end)
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
    }
  }
  ColumnLayout {
    visible: !isLoading
    anchors.fill: parent
    spacing: 0
    Cdt.Pane {
      id: searchBar
      Layout.fillWidth: true
      color: Theme.colorBgMedium
      padding: Theme.basePadding
      visible: false
      Keys.onEscapePressed: closeSearchBar(true)
      RowLayout {
        anchors.fill: parent
        Cdt.TextField {
          id: searchOutputTextField
          Layout.fillWidth: true
          placeholderText: "Search text"
          onDisplayTextChanged: controller.search(displayText, root.getText())
          KeyNavigation.down: textArea
          KeyNavigation.right: searchPrevBtn
          function goToSearchResult(event) {
            if (event.modifiers & Qt.ShiftModifier) {
              controller.previousResult();
            } else {
              controller.nextResult();
            }
          }
          Keys.onReturnPressed: (event) => goToSearchResult(event)
          Keys.onEnterPressed: (event) => goToSearchResult(event)
        }
        Cdt.Text {
          text: controller.searchResultsCount
        }
        Cdt.IconButton {
          id: searchPrevBtn
          buttonIcon: "arrow_upward"
          enabled: !controller.searchResultsEmpty
          onClicked: controller.previousResult()
          KeyNavigation.down: textArea
          KeyNavigation.right: searchNextBtn
        }
        Cdt.IconButton {
          id: searchNextBtn
          buttonIcon: "arrow_downward"
          enabled: !controller.searchResultsEmpty
          onClicked: controller.nextResult()
          KeyNavigation.down: textArea
        }
      }
    }
    Cdt.Pane {
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: root.color
      focus: !searchBar.visible
      ScrollView {
        anchors.fill: parent
        focus: true
        Keys.forwardTo: [root]
        TextArea {
          id: textArea
          property var allowedKeys: new Set([
            Qt.Key_Left,
            Qt.Key_Right,
            Qt.Key_Up,
            Qt.Key_Down,
            Qt.Key_Home,
            Qt.Key_End,
          ])

          selectByMouse: true
          selectionColor: Theme.colorHighlight
          color: Theme.colorText
          leftPadding: root.innerPadding
          rightPadding: root.innerPadding
          topPadding: root.innerPadding
          bottomPadding: root.innerPadding
          textFormat: TextEdit.PlainText
          focus: true
          background: Rectangle {
            color: "transparent"
          }
          wrapMode: TextArea.WordWrap
          onCursorPositionChanged: controller.saveCursorPosition(cursorPosition)
          // Make text area effectively readOnly but don't hide the cursor and
          // allow navigating it using the cursor
          Keys.onPressed: event => {
            if (event.key === Qt.Key_Escape) {
              if (searchBar.visible) {
                closeSearchBar(true);
              } else {
                textArea.deselect();
              }
            } else if (event.key === Qt.Key_Return) {
              controller.openFileLinkAtCursor();
            } else if (event.key === Qt.Key_PageUp) {
              controller.movePage(root.getText(), true);
            } else if (event.key === Qt.Key_PageDown) {
              controller.movePage(root.getText(), false);
            }
            if (!allowedKeys.has(event.key) &&
                !event.matches(StandardKey.Copy) &&
                !event.matches(StandardKey.SelectAll)) {
              event.accepted = true;
            }
          }
          onPressed: e => Common.handleRightClick(textArea, contextMenu, e)
          Menu {
            id: contextMenu
            MenuItem {
              text: "Copy"
              enabled: textArea.activeFocus
              shortcut: "Ctrl+C"
              onTriggered: textArea.copy()
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
              onTriggered: controller.goToPreviousCursorPosition();
            }
            MenuItem {
              text: "Next Cursor Position"
              enabled: textArea.activeFocus
              shortcut: "Ctrl+Alt+Right"
              onTriggered: controller.goToNextCursorPosition()
            }
            MenuItem {
              text: "Open File In Editor"
              enabled: controller.detectFileLinks && textArea.activeFocus && controller.isCursorOnLink
              shortcut: "Enter"
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
