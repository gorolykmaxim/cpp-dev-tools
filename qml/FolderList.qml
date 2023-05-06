import QtQuick
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt

Loader {
  id: root
  property var foldersModel: null
  signal folderAdded(folder: string)
  signal folderRemoved(folder: string)
  signal back()
  focus: true
  anchors.fill: parent
  sourceComponent: listView
  Component {
    id: listView
    ColumnLayout {
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
            placeholderText: "Search folder"
            Layout.fillWidth: true
            KeyNavigation.right: addBtn
            list: folderList
            listModel: foldersModel
            focus: true
          }
          Cdt.Button {
            id: addBtn
            text: "Add Folder"
            onClicked: root.sourceComponent = chooseFolderView
            KeyNavigation.right: backBtn
          }
          Cdt.Button {
            id: backBtn
            text: "Back"
            onClicked: root.back()
          }
        }
      }
      Cdt.PlaceholderText {
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: folderList.count === 0
        text: "Add folders by clicking on '" + addBtn.text + "' button"
      }
      Cdt.Pane {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: Theme.colorBgDark
        visible: folderList.count > 0
        Cdt.TextList {
          id: folderList
          model: foldersModel
          anchors.fill: parent
          onItemRightClicked: contextMenu.open()
        }
      }
      Menu {
        id: contextMenu
        MenuItem {
          text: "Remove Folder"
          enabled: input.activeFocus
          shortcut: "Alt+Shift+D"
          onTriggered: folderList.ifCurrentItem('title', folderRemoved)
        }
      }
    }
  }
  Component {
    id: chooseFolderView
    Cdt.ChooseFile {
      allowCreating: false
      chooseFolder: true
      onFileChosen: (file) => {
        folderAdded(file);
        root.sourceComponent = listView;
      }
      onCancelled: root.sourceComponent = listView
    }
  }
}
