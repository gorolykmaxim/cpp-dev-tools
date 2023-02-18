import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
  anchors.fill: parent
  TextWidget {
    text: dTitle ?? ""
    font.bold: true
  }
  RowLayout {
    Layout.fillHeight: true
    IconWidget {
      icon: dError ? "error" : "help"
      iconSize: 48
      color: dError ? "red" : colorText
      Layout.alignment: Qt.AlignTop
    }
    ScrollView {
      Layout.fillWidth: true
      Layout.fillHeight: true
      ReadOnlyTextAreaWidget {
        textData: dText ?? ""
        KeyNavigation.down: dialogCancel.visible ? dialogCancel : dialogOk
      }
    }
  }
  Row {
    spacing: basePadding
    Layout.alignment: Qt.AlignRight
    ButtonWidget {
      id: dialogCancel
      text: "Cancel"
      focus: visible
      visible: dCancellable ?? false
      KeyNavigation.right: dialogOk
      onClicked: dialog.reject()
    }
    ButtonWidget {
      id: dialogOk
      text: "OK"
      focus: !dialogCancel.visible
      onClicked: dialog.accept()
    }
  }
}
