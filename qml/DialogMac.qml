import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
  id: dialog
  title: dialogDataTitle ?? ""
  width: parent.width / 2
  modal: true
  visible: dialogDataVisible ?? false
  x: (parent.width - width) / 2
  ColumnLayout {
    anchors.fill: parent
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
        highlighted: true
        onClicked: {
          dialog.accept();
          core.OnDialogResult(true);
        }
      }
    }
  }
}
