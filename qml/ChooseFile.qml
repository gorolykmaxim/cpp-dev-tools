import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import cdt
import "." as Cdt

ColumnLayout {
  id: root
  property string title: "Choose File"
  signal fileChosen(string path)
  signal cancelled()
  Component.onCompleted: {
    input.forceActiveFocus();
    appWindow.title = title;
  }
  Connections {
      target: alertDialog
      function onAccepted() {
          controller.CreateFile();
      }
  }
  ChooseFileController {
    id: controller
    onWillCreateFile: alertDialog.display("Create new folder?",
      "Do you want to create a new folder: " + controller.resultPath + "?")
    onFileChosen: root.fileChosen(controller.resultPath)
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
