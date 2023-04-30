import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import cdt
import "." as Cdt

ColumnLayout {
  id: root
  property alias allowCreating: controller.allowCreating
  property alias chooseFolder: controller.chooseFolder
  signal fileChosen(string path)
  signal cancelled()
  Component.onCompleted: controller.initialize()
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
    focus: true
    RowLayout {
      anchors.fill: parent
      Cdt.ListSearch {
        id: input
        text: controller.path
        Layout.fillWidth: true
        list: suggestionList
        focus: true
        KeyNavigation.right: openBtn
        onDisplayTextChanged: controller.path = displayText
        onEnterPressed: suggestionList.ifCurrentItem('idx', controller.pickSuggestion)
        onCtrlEnterPressed: controller.openOrCreateFile()
      }
      Cdt.Button {
        id: openBtn
        text: "Open"
        enabled: controller.canOpen
        KeyNavigation.right: cancelBtn
        onClicked: controller.openOrCreateFile()
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
      onItemLeftClicked: suggestionList.ifCurrentItem('idx', controller.pickSuggestion)
    }
  }
}
