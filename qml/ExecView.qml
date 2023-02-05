import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Common.js" as Cmn

PaneWidget {
  Component.onCompleted: searchExecTextField.forceActiveFocus()
  anchors.fill: parent
  color: colorBgDark
  RowLayout {
    anchors.fill: parent
    spacing: 0
    ColumnLayout {
      Layout.maximumWidth: 300
      Layout.minimumWidth: 300
      spacing: 0
      PaneWidget {
        Layout.fillWidth: true
        color: colorBgMedium
        padding: basePadding
        TextFieldWidget {
          id: searchExecTextField
          anchors.fill: parent
          text: vExecFilter || ""
          placeholderText: "Search execution"
          onDisplayTextChanged: core.OnAction("vaExecFilterChanged",
                                              [displayText])
          Keys.onDownPressed: execList.incrementCurrentIndex()
          Keys.onUpPressed: execList.decrementCurrentIndex()
          KeyNavigation.right: searchOutputTextField
        }
      }
      TextListWidget {
        id: execList
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: vExecs
        onCurrentIndexChanged: Cmn.onListAction(execList, "vaExecSelected",
                                                "id")
      }
    }
    Rectangle {
      Layout.fillHeight: true
      width: 1
      color: colorBgBlack
    }
    ColumnLayout {
      Layout.fillWidth: true
      spacing: 0
      PaneWidget {
        Layout.fillWidth: true
        color: colorBgMedium
        padding: basePadding
        ColumnLayout {
          spacing: 0
          TextWidget {
            Layout.fillWidth: true
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
        TextFieldWidget {
          id: searchOutputTextField
          anchors.fill: parent
          placeholderText: "Search in output"
          KeyNavigation.down: execOutputTextArea
        }
      }
      ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        ReadOnlyTextAreaWidget {
          id: execOutputTextArea
          leftPadding: basePadding
          rightPadding: basePadding
          topPadding: basePadding
          bottomPadding: basePadding
          text: vExecOutput || ""
          onTextChanged: {
            cursorPosition = length;
          }
          KeyNavigation.left: searchExecTextField
        }
      }
    }
  }
}
