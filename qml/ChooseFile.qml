import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import cdt
import "." as Cdt

ColumnLayout {
  id: root
  signal fileChosen(string path)
  signal cancelled()
  Component.onCompleted: {
    input.forceActiveFocus();
  }
  ChooseFileController {
    id: controller
    onFileChosen: (path) => root.fileChosen(path)
  }
  anchors.fill: parent
  spacing: 0
  Cdt.Pane {
    Layout.fillWidth: true
    color: Theme.colorBgMedium
    padding: Theme.basePadding
    RowLayout {
      anchors.fill: parent
      Cdt.ListSearch {
        id: input
        text: controller.path
        Layout.fillWidth: true
        list: suggestionList
        KeyNavigation.right: openBtn
        onDisplayTextChanged: controller.path = displayText
        onEnterPressed: suggestionList.ifCurrentItem('idx', controller.PickSuggestion)
        onCtrlEnterPressed: controller.OpenOrCreateFile()
      }
      Cdt.Button {
        id: openBtn
        text: "Open"
        KeyNavigation.right: cancelBtn
        onClicked: controller.OpenOrCreateFile()
      }
      Cdt.Button {
        id: cancelBtn
        text: "Cancel"
        onClicked: root.cancelled()
      }
    }
  }
  Cdt.Pane {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Theme.colorBgDark
    Cdt.TextList {
      id: suggestionList
      anchors.fill: parent
      model: controller.suggestions
      onItemLeftClicked: suggestionList.ifCurrentItem('idx', controller.PickSuggestion)
    }
  }
}
