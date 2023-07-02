import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

Cdt.Pane {
  required property string windowTitle
  signal filterChosen(string filter)
  signal back()
  anchors.fill: parent
  focus: true
  Component.onCompleted: viewSystem.windowTitle = windowTitle
  Keys.onEscapePressed: back()
  RowLayout {
    anchors.centerIn: parent
    width: Theme.centeredViewWidth
    spacing: Theme.basePadding
    Cdt.TextField {
      id: input
      Layout.fillWidth: true
      focus: true
      placeholderText: "Test filter"
      Keys.onEnterPressed: runBtn.clicked()
      Keys.onReturnPressed: runBtn.clicked()
      KeyNavigation.right: runBtn
    }
    Cdt.Button {
      id: runBtn
      text: "Run"
      onClicked: filterChosen(input.displayText)
      KeyNavigation.right: backBtn
    }
    Cdt.Button {
      id: backBtn
      text: "Back"
      onClicked: back()
    }
  }
}
