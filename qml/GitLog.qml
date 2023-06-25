import QtQuick
import QtQuick.Layouts
import Qt.labs.platform
import cdt
import "." as Cdt

Loader {
  id: root
  anchors.fill: parent
  focus: true
  sourceComponent: listView
  GitLogModel {
    id: logModel
  }
  Component {
    id: listView
    ColumnLayout {
      anchors.fill: parent
      spacing: 0
      Component.onCompleted: logModel.load()
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
        onItemRightClicked: contextMenu.open()
      }
      Menu {
        id: contextMenu
        MenuItem {
          text: "Checkout"
          shortcut: "Alt+C"
          enabled: gitLogList.activeFocus
          onTriggered: logModel.checkout()
        }
        MenuItem {
          text: "Cherry-Pick"
          shortcut: "Alt+P"
          enabled: gitLogList.activeFocus
          onTriggered: logModel.cherryPick()
        }
        MenuItem {
          text: "New Branch"
          shortcut: "Alt+N"
          enabled: gitLogList.activeFocus
          onTriggered: root.sourceComponent = createView
        }
      }
    }
  }
  Component {
    id: createView
    Cdt.NewGitBranch {
      branchBasis: logModel.selectedCommitSha
      onBack: root.sourceComponent = listView
    }
  }
}
