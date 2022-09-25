import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
  id: dialog
  width: parent.width / 2
  modal: true
  visible: dialogDataVisible ?? false
  anchors.centerIn: parent
  background: Rectangle {
    color: colorBgMedium
    radius: baseRadius
  }
  ColumnLayout {
    anchors.fill: parent
    TextWidget {
      text: dialogDataTitle ?? ""
      font.bold: true
    }
    RowLayout {
      IconWidget {
        icon: dialogDataError ? "error" : "help"
        color: dialogDataError ? "red" : palette.text
        Layout.alignment: Qt.AlignTop
      }
      ScrollView {
        Layout.fillWidth: true
        Layout.maximumHeight: 300
        ReadOnlyTextAreaWidget {
          text: dialogDataText ?? ""
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
        visible: dialogDataCancellable ?? false
        KeyNavigation.right: dialogOk
        onClicked: {
          dialog.reject();
          core.OnDialogResult(false);
        }
      }
      ButtonWidget {
        id: dialogOk
        text: "OK"
        focus: !dialogCancel.visible
        onClicked: {
          dialog.accept();
          core.OnDialogResult(true);
        }
      }
    }
  }
}
