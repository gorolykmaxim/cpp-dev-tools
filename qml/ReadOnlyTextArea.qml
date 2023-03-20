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
  property string text: ""
  property string color: "transparent"
  property var textFormat: TextEdit.PlainText
  property real innerPadding: 0
  onTextChanged: {
    textArea.text = text;
    controller.ResetCursorPositionHistory();
    if (cursorFollowEnd) {
      textArea.cursorPosition = textArea.length;
    }
    if (searchBar.visible) {
      controller.Search(searchOutputTextField.displayText,
                        textArea.getText(0, textArea.length))
    }
  }
  function openSearchBar() {
    if (!searchable) {
      return;
    }
    const position = textArea.selectionStart;
    searchOutputTextField.text = textArea.selectedText;
    searchBar.visible = true;
    controller.GoToResultWithStartAt(position);
  }
  function closeSearchBar() {
    searchBar.visible = false;
  }
  TextAreaController {
    id: controller
    onSelectText: (start, end) => textArea.select(start, end)
    onChangeCursorPosition: pos => textArea.cursorPosition = pos
  }
  ColumnLayout {
    anchors.fill: parent
    spacing: 0
    Cdt.Pane {
      id: searchBar
      Layout.fillWidth: true
      color: Theme.colorBgMedium
      padding: Theme.basePadding
      visible: false
      focus: visible
      Keys.onEscapePressed: closeSearchBar(true)
      RowLayout {
        anchors.fill: parent
        Cdt.TextField {
          id: searchOutputTextField
          Layout.fillWidth: true
          placeholderText: "Search text"
          onDisplayTextChanged: controller.Search(displayText,
                                                  textArea.getText(0, textArea.length))
          // This is bound to "visible" and not just set to true in order to
          // trigger its re-evaluation when searchBar becomes visible.
          // Otheriwse if the last time when a searchBar got hidden one of its
          // arrow buttons had focus - when the next time searchBar gets
          // displayed that button would still have focus (even in case if it
          // is now disabled and can't be interacted with, which will trap focus)
          focus: visible
          KeyNavigation.down: textArea
          KeyNavigation.right: searchPrevBtn
          function goToSearchResult(event) {
            if (event.modifiers & Qt.ShiftModifier) {
              controller.PreviousResult();
            } else {
              controller.NextResult();
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
          onClicked: controller.PreviousResult()
          KeyNavigation.down: textArea
          KeyNavigation.right: searchNextBtn
        }
        Cdt.IconButton {
          id: searchNextBtn
          buttonIcon: "arrow_downward"
          enabled: !controller.searchResultsEmpty
          onClicked: controller.NextResult()
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
          textFormat: root.textFormat
          focus: true
          background: Rectangle {
            color: "transparent"
          }
          wrapMode: TextArea.WordWrap
          onCursorPositionChanged: controller.SaveCursorPosition(cursorPosition)
          // Make text area effectively readOnly but don't hide the cursor and
          // allow navigating it using the cursor
          Keys.onPressed: event => {
            if (event.key === Qt.Key_Escape) {
              if (searchBar.visible) {
                closeSearchBar(true);
              } else {
                textArea.deselect();
              }
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
              onTriggered: controller.GoToPreviousCursorPosition();
            }
            MenuItem {
              text: "Next Cursor Position"
              enabled: textArea.activeFocus
              shortcut: "Ctrl+Alt+Right"
              onTriggered: controller.GoToNextCursorPosition()
            }
          }
        }
      }
    }
  }
}
