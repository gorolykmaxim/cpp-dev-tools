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
          KeyNavigation.right: execOutputTextArea
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
    ColumnLayout {
      Layout.fillWidth: true
      spacing: 0
      PaneWidget {
        Layout.fillWidth: true
        color: colorBgMedium
        padding: basePadding
        RowLayout {
          IconWidget {
            icon: vExecIcon || ""
            color: vExecIconColor || colorText
          }
          Column {
            Layout.fillWidth: true
            TextWidget {
              text: vExecName || ""
            }
            TextWidget {
              text: vExecCmd || ""
              color: colorSubText
            }
          }
        }
      }
      ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        ReadOnlyTextAreaWidget {
          id: execOutputTextArea
          text: vExecOutput || ""
          onTextChanged: {
            cursorPosition = length;
          }
        }
      }
    }
  }
}
