import QtQuick
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt

Loader {
  id: root
  property var foldersModel: null
  signal back()
  focus: true
  anchors.fill: parent
  sourceComponent: listView
  Component {
    id: listView
    ColumnLayout {
      anchors.fill: parent
      spacing: 0
      Keys.onEscapePressed: root.back()
      Component.onCompleted: foldersModel.load()
      Cdt.Pane {
        Layout.fillWidth: true
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
      Cdt.TextList {
        id: folderList
        model: foldersModel
        Layout.fillWidth: true
        Layout.fillHeight: true
        onItemRightClicked: contextMenu.open()
      }
      Menu {
        id: contextMenu
        MenuItem {
          text: "Remove Folder"
          enabled: input.activeFocus || folderList.activeFocus
          shortcut: gSC("FolderList", "Remove Folder")
          onTriggered: foldersModel.removeSelectedFolder()
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
        foldersModel.addFolder(file);
        root.sourceComponent = listView;
      }
      onCancelled: root.sourceComponent = listView
    }
  }
}
