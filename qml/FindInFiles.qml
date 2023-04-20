import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "." as Cdt
import cdt

SplitView {
  anchors.fill: parent
  handle: Rectangle {
    implicitWidth: Theme.basePadding / 2
    implicitHeight: Theme.basePadding / 2
    color: Theme.colorBgBlack
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
            focus: true
            placeholderText: "Search"
            Layout.fillWidth: true
            KeyNavigation.down: matchCaseCheck.visible ? matchCaseCheck : searchResultsList
            KeyNavigation.right: searchBtn
          }
          Cdt.Button {
            id: searchBtn
            text: "Search"
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
          KeyNavigation.down: matchWholeWordCheck
          KeyNavigation.right: filePreviewArea
        }
        Cdt.CheckBox {
          id: matchWholeWordCheck
          text: "Match Whole Word"
          visible: advancedBtn.checked
          KeyNavigation.down: regularExpressionCheck
          KeyNavigation.right: filePreviewArea
        }
        Cdt.CheckBox {
          id: regularExpressionCheck
          text: "Regular Expression"
          visible: advancedBtn.checked
          KeyNavigation.down: includeExternalSearchFoldersCheck
          KeyNavigation.right: filePreviewArea
        }
        Cdt.CheckBox {
          id: includeExternalSearchFoldersCheck
          text: "Include External Search Folders"
          visible: advancedBtn.checked
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
          text: "Searching..."
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
        KeyNavigation.right: filePreviewArea
        KeyNavigation.up: fileToExcludeInput.visible ? fileToExcludeInput : searchInput
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
