import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Common.js" as Cmn

RowLayout {
  Component.onCompleted: searchExecTextField.forceActiveFocus()
  anchors.fill: parent
  spacing: 0
  TextWidget {
    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
    visible: vExecsEmpty
    text: "No tasks have been executed yet"
  }
  ColumnLayout {
    visible: !vExecsEmpty
    Layout.maximumWidth: 300
    Layout.minimumWidth: 300
    spacing: 0
    PaneWidget {
      Layout.fillWidth: true
      color: colorBgMedium
      padding: basePadding
      ListSearchWidget {
        id: searchExecTextField
        anchors.fill: parent
        text: vExecFilter || ""
        placeholderText: "Search execution"
        list: execList
        changeEventType: "vaExecFilterChanged"
        KeyNavigation.right: searchOutputTextField
      }
    }
    PaneWidget {
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: colorBgDark
      TextListWidget {
        id: execList
        anchors.fill: parent
        model: vExecs
        elide: Text.ElideLeft
        onCurrentIndexChanged: Cmn.onListAction(execList, "vaExecSelected",
                                                "id")
      }
    }
  }
  Rectangle {
    visible: !vExecsEmpty
    Layout.fillHeight: true
    width: 1
    color: colorBgBlack
  }
  ColumnLayout {
    visible: !vExecsEmpty
    Layout.fillWidth: true
    spacing: 0
    PaneWidget {
      Layout.fillWidth: true
      color: colorBgMedium
      padding: basePadding
      ColumnLayout {
        spacing: 0
        width: parent.width
        TextWidget {
          Layout.fillWidth: true
          elide: Text.ElideLeft
          text: vExecCmd || ""
        }
        TextWidget {
          Layout.fillWidth: true
          text: vExecStatus || ""
          color: colorSubText
        }
      }
    }
    Rectangle {
      Layout.fillWidth: true
      height: 1
      color: colorBgBlack
    }
    PaneWidget {
      Layout.fillWidth: true
      color: colorBgMedium
      padding: basePadding
      RowLayout {
        anchors.fill: parent
        TextFieldWidget {
          id: searchOutputTextField
          text: vExecOutputFilter || ""
          placeholderText: "Search in output"
          onDisplayTextChanged: core.OnAction("vaSearchExecOutput",
                                              [displayText])
          KeyNavigation.down: execOutputTextArea
          KeyNavigation.right: searchPrevBtn
          function goToSearchResult(event) {
            if (event.modifiers & Qt.ControlModifier) {
              core.OnAction("vaPrevExecOutputSearchResult", [])
            } else {
              core.OnAction("vaNextExecOutputSearchResult", [])
            }
          }
          Keys.onReturnPressed: goToSearchResult(event)
          Keys.onEnterPressed: goToSearchResult(event)
          Layout.fillWidth: true
        }
        TextWidget {
          text: vExecOutputSearchResults
        }
        IconButtonWidget {
          id: searchPrevBtn
          buttonIcon: "arrow_upward"
          enabled: !vExecOutputNoSearchResults
          onClicked: core.OnAction("vaPrevExecOutputSearchResult", [])
          KeyNavigation.right: searchNextBtn
          KeyNavigation.down: execOutputTextArea
        }
        IconButtonWidget {
          id: searchNextBtn
          buttonIcon: "arrow_downward"
          enabled: !vExecOutputNoSearchResults
          onClicked: core.OnAction("vaNextExecOutputSearchResult", [])
          KeyNavigation.down: execOutputTextArea
        }
      }
    }
    PaneWidget {
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: colorBgDark
      ScrollView {
        anchors.fill: parent
        ReadOnlyTextAreaWidget {
          id: execOutputTextArea
          leftPadding: basePadding
          rightPadding: basePadding
          topPadding: basePadding
          bottomPadding: basePadding
          text: vExecOutput || ""
          textFormat: TextEdit.RichText
          selectionStart: vExecOutputSearchResultStart
          selectionEnd: vExecOutputSearchResultEnd
          onTextChanged: {
            cursorPosition = length;
          }
          KeyNavigation.left: searchExecTextField
        }
      }
    }
  }
}
