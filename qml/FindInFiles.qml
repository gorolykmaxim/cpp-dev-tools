import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt

SplitView {
  anchors.fill: parent
  handle: Rectangle {
    implicitWidth: Theme.basePadding / 2
    implicitHeight: Theme.basePadding / 2
    color: parent.resizing ? Theme.colorHighlight : Theme.colorBgBlack
  }
  FindInFilesController {
    id: controller
  }
  ColumnLayout {
    SplitView.minimumWidth: 300
    SplitView.fillHeight: true
    spacing: 0
    Cdt.Pane {
      focus: true
      color: Theme.colorBgMedium
      padding: Theme.basePadding
      Layout.fillWidth: true
      Column {
        width: parent.width
        spacing: Theme.basePadding
        RowLayout {
          width: parent.width
          Cdt.TextField {
            id: searchInput
            text: controller.searchTerm
            onDisplayTextChanged: controller.searchTerm = displayText
            focus: true
            placeholderText: "Search"
            Layout.fillWidth: true
            KeyNavigation.down: matchCaseCheck.visible ? matchCaseCheck : searchResultsList
            KeyNavigation.right: searchBtn
            Keys.onEnterPressed: controller.search()
            Keys.onReturnPressed: controller.search()
          }
          Cdt.Button {
            id: searchBtn
            text: "Search"
            onClicked: controller.search()
            KeyNavigation.down: matchCaseCheck.visible ? matchCaseCheck : searchResultsList
            KeyNavigation.right: advancedBtn
          }
          Cdt.IconButton {
            id: advancedBtn
            buttonIcon: "more_vert"
            checkable: true
            KeyNavigation.down: matchCaseCheck.visible ? matchCaseCheck : searchResultsList
            KeyNavigation.right: filePreviewArea
          }
        }
        Cdt.CheckBox {
          id: matchCaseCheck
          text: "Match Case"
          visible: advancedBtn.checked
          checked: controller.options.matchCase
          onCheckedChanged: controller.options.matchCase = checked
          KeyNavigation.down: matchWholeWordCheck
          KeyNavigation.right: filePreviewArea
        }
        Cdt.CheckBox {
          id: matchWholeWordCheck
          text: "Match Whole Word"
          visible: advancedBtn.checked
          checked: controller.options.matchWholeWord
          onCheckedChanged: controller.options.matchWholeWord = checked
          KeyNavigation.down: regularExpressionCheck
          KeyNavigation.right: filePreviewArea
        }
        Cdt.CheckBox {
          id: regularExpressionCheck
          text: "Regular Expression"
          visible: advancedBtn.checked
          checked: controller.options.regexp
          onCheckedChanged: controller.options.regexp = checked
          KeyNavigation.down: includeExternalSearchFoldersCheck
          KeyNavigation.right: filePreviewArea
        }
        Cdt.CheckBox {
          id: includeExternalSearchFoldersCheck
          text: "Include External Search Folders"
          visible: advancedBtn.checked
          checked: controller.options.includeExternalSearchFolders
          onCheckedChanged: controller.options.includeExternalSearchFolders = checked
          KeyNavigation.down: fileToIncludeInput
          KeyNavigation.right: filePreviewArea
        }
        Cdt.TextField {
          id: fileToIncludeInput
          placeholderText: "Files To Include"
          width: parent.width
          visible: advancedBtn.checked
          KeyNavigation.down: fileToExcludeInput
          KeyNavigation.right: filePreviewArea
        }
        Cdt.TextField {
          id: fileToExcludeInput
          placeholderText: "Files To Exclude"
          width: parent.width
          visible: advancedBtn.checked
          KeyNavigation.down: searchResultsList
          KeyNavigation.right: filePreviewArea
        }
        Cdt.Text {
          text: controller.searchStatus
          color: Theme.colorSubText
        }
      }
    }
    Cdt.Pane {
      color: Theme.colorBgDark
      Layout.fillWidth: true
      Layout.fillHeight: true
      Cdt.TextList {
        id: searchResultsList
        anchors.fill: parent
        model: controller.results
        elide: Text.ElideRight
        highlightCurrentItemWithoutFocus: false
        onItemSelected: ifCurrentItem('idx', controller.selectResult)
        onItemRightClicked: {
          forceActiveFocus();
          contextMenu.open();
        }
        Keys.onEnterPressed: controller.openSelectedResultInEditor()
        Keys.onReturnPressed: controller.openSelectedResultInEditor()
        KeyNavigation.right: filePreviewArea
        KeyNavigation.up: fileToExcludeInput.visible ? fileToExcludeInput : searchInput
        Menu {
          id: contextMenu
          MenuItem {
            text: "Open In Editor"
            shortcut: "Enter"
            enabled: searchResultsList.activeFocus
            onTriggered: controller.openSelectedResultInEditor()
          }
        }
      }
    }
  }
  ReadOnlyTextArea {
    id: filePreviewArea
    SplitView.fillWidth: true
    SplitView.fillHeight: true
    color: Theme.colorBgDark
    innerPadding: Theme.basePadding
    detectFileLinks: false
    searchable: true
  }
}
