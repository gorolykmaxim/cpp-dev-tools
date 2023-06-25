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
        Layout.minimumWidth: 100
        onDisplayTextChanged: logModel.branch = displayText
        placeholderText: "Branch"
        KeyNavigation.right: searchTextField
        Keys.onEnterPressed: logModel.load()
        Keys.onReturnPressed: logModel.load()
      }
      Cdt.TextField {
        id: searchTextField
        text: logModel.searchTerm
        onDisplayTextChanged: logModel.searchTerm = displayText
        placeholderText: "Search term"
        focus: true
        Layout.fillWidth: true
        KeyNavigation.right: searchBtn
        Keys.onEnterPressed: logModel.load()
        Keys.onReturnPressed: logModel.load()
      }
      Cdt.Button {
        id: searchBtn
        text: "Search"
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
