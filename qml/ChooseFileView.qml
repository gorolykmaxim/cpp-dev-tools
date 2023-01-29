import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
  anchors.fill: parent
  spacing: 0
  PaneWidget {
    Layout.fillWidth: true
    color: colorBgMedium
    focus: true
    padding: basePadding
    RowLayout {
      anchors.fill: parent
      TextFieldWidget {
        id: input
        text: vPath || ""
        focus: true
        Layout.fillWidth: true
        KeyNavigation.right: openBtn
        onDisplayTextChanged: core.OnAction("vaPathChanged", [displayText])
        Keys.onReturnPressed: e => {
          if (e.modifiers & Qt.ControlModifier) {
            core.OnAction("vaOpenOrCreate", []);
          } else {
            core.OnAction("vaSuggestionPicked", [list.currentIndex]);
          }
        }
        Keys.onEnterPressed: core.OnAction("vaSuggestionPicked",
                                           [list.currentIndex])
        Keys.onDownPressed: list.incrementCurrentIndex()
        Keys.onUpPressed: list.decrementCurrentIndex()
      }
      ButtonWidget {
        id: openBtn
        text: "Open"
        KeyNavigation.right: cancelBtn
        onClicked: core.OnAction("vaOpenOrCreate", [])
      }
      ButtonWidget {
        id: cancelBtn
        text: "Cancel"
        onClicked: core.OnAction("vaOpeningCancelled", [])
      }
    }
  }
  PaneWidget {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: colorBgDark
    TextListWidget {
      id: list
      anchors.fill: parent
      model: vSuggestions
      onItemClicked: core.OnAction("vaSuggestionPicked", [list.currentIndex])
    }
  }
}
