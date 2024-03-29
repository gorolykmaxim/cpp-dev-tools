import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt

SplitView {
  id: root
  anchors.fill: parent
  handle: Cdt.SplitViewHandle {
    viewId: "FindInFiles"
    view: root
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
            checked: controller.options.expanded
            onCheckedChanged: controller.options.expanded = checked
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
          KeyNavigation.down: excludeGitIgnoredFilesCheck
          KeyNavigation.right: filePreviewArea
        }
        Cdt.CheckBox {
          id: excludeGitIgnoredFilesCheck
          text: "Exclude Git Ignored Files"
          visible: advancedBtn.checked
          checked: controller.options.excludeGitIgnoredFiles
          onCheckedChanged: controller.options.excludeGitIgnoredFiles = checked
          KeyNavigation.down: fileToIncludeInput
          KeyNavigation.right: filePreviewArea
        }
        Cdt.TextField {
          id: fileToIncludeInput
          text: controller.options.filesToInclude
          onDisplayTextChanged: controller.options.filesToInclude = displayText
          placeholderText: "Files To Include"
          width: parent.width
          visible: advancedBtn.checked
          KeyNavigation.down: fileToExcludeInput
          KeyNavigation.right: filePreviewArea
          Keys.onEnterPressed: controller.search()
          Keys.onReturnPressed: controller.search()
        }
        Cdt.TextField {
          id: fileToExcludeInput
          text: controller.options.filesToExclude
          onDisplayTextChanged: controller.options.filesToExclude = displayText
          placeholderText: "Files To Exclude"
          width: parent.width
          visible: advancedBtn.checked
          KeyNavigation.down: searchResultsList
          KeyNavigation.right: filePreviewArea
          Keys.onEnterPressed: controller.search()
          Keys.onReturnPressed: controller.search()
        }
        Cdt.Text {
          text: controller.searchStatus
          color: Theme.colorPlaceholder
        }
      }
    }
    Rectangle {
      Layout.fillWidth: true
      height: 1
      color: Theme.colorBorder
    }
    Cdt.TextList {
      id: searchResultsList
      Layout.fillWidth: true
      Layout.fillHeight: true
      model: controller.results
      elide: Text.ElideRight
      highlightCurrentItemWithoutFocus: false
      onItemRightClicked: contextMenu.open()
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
  ColumnLayout {
    SplitView.fillWidth: true
    SplitView.fillHeight: true
    spacing: 0
    Cdt.Text {
      Layout.fillWidth: true
      padding: Theme.basePadding
      text: controller.selectedFilePath
      elide: Text.ElideLeft
    }
    Rectangle {
      Layout.fillWidth: true
      height: 1
      color: Theme.colorBorder
    }
    Cdt.BigTextArea {
      id: filePreviewArea
      Layout.fillWidth: true
      Layout.fillHeight: true
      text: controller.selectedFileContent
      cursorPosition: controller.selectedFileCursorPosition
      displayLineNumbers: true
      highlightCurrentLineWithoutFocus: true
      formatters: [controller.formatter]
    }
  }
}
