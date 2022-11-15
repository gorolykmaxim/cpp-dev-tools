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
    IconWidget {
      icon: dError ? "error" : "help"
      color: dError ? "red" : colorText
      Layout.alignment: Qt.AlignTop
    }
    ScrollView {
      Layout.fillWidth: true
      Layout.maximumHeight: 300
      ReadOnlyTextAreaWidget {
        text: dText ?? ""
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
      onClicked: {
        dialog.reject();
        core.OnUserAction("dialogSlot", "cancel", []);
      }
    }
    ButtonWidget {
      id: dialogOk
      text: "OK"
      focus: !dialogCancel.visible
      onClicked: {
        dialog.accept();
        core.OnUserAction("dialogSlot", "ok", []);
      }
    }
  }
}
