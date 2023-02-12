import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "Common.js" as Cmn

ColumnLayout {
  Component.onCompleted: input.forceActiveFocus()
  anchors.fill: parent
  spacing: 0
  PaneWidget {
    Layout.fillWidth: true
    color: colorBgMedium
    padding: basePadding
    RowLayout {
      anchors.fill: parent
      ListSearchWidget {
        id: input
        text: vPath || ""
        Layout.fillWidth: true
        list: suggestionList
        changeEventType: "vaPathChanged"
        enterEventType: "vaSuggestionPicked"
        ctrlEnterEventType: "vaOpenOrCreate"
        listIdFieldName: "idx"
        KeyNavigation.right: openBtn
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
      id: suggestionList
      anchors.fill: parent
      model: vSuggestions
      onItemLeftClicked: Cmn.onListAction(suggestionList, "vaSuggestionPicked",
                                          "idx")
    }
  }
}
