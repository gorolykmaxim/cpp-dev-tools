import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
  anchors.fill: parent
  TextWidget {
    text: dataDialogTitle ?? ""
    font.bold: true
  }
  RowLayout {
    IconWidget {
      icon: dataDialogError ? "error" : "help"
      color: dataDialogError ? "red" : colorText
      Layout.alignment: Qt.AlignTop
    }
    ScrollView {
      Layout.fillWidth: true
      Layout.maximumHeight: 300
      ReadOnlyTextAreaWidget {
        text: dataDialogText ?? ""
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
      visible: dataDialogCancellable ?? false
      KeyNavigation.right: dialogOk
      onClicked: {
        dialog.reject();
        core.OnUserAction("dialogSlot", "actionDialogCancel", []);
      }
    }
    ButtonWidget {
      id: dialogOk
      text: "OK"
      focus: !dialogCancel.visible
      onClicked: {
        dialog.accept();
        core.OnUserAction("dialogSlot", "actionDialogOk", []);
      }
    }
  }
}
