import QtQuick
import QtQuick.Layouts
import cdt
import "." as Cdt

ColumnLayout {
  anchors.fill: parent
  spacing: 0
  GitLogModel {
    id: logModel
  }
  Cdt.Pane {
    id: controlsPane
    Layout.fillWidth: true
    padding: Theme.basePadding
    KeyNavigation.down: gitLogList
    RowLayout {
      anchors.fill: parent
      spacing: Theme.basePadding
      Cdt.TextField {
        text: logModel.branch
        onDisplayTextChanged: logModel.branch = displayText
        placeholderText: "Branch"
        Layout.fillWidth: true
        KeyNavigation.right: searchBtn
        Keys.onEnterPressed: logModel.load()
        Keys.onReturnPressed: logModel.load()
      }
      Cdt.Button {
        id: searchBtn
        text: "Search"
        focus: true
        onClicked: logModel.load()
      }
    }
  }
  Cdt.TextList {
    id: gitLogList
    Layout.fillWidth: true
    Layout.fillHeight: true
    focus: true
    model: logModel
    highlightCurrentItemWithoutFocus: false
  }
}
