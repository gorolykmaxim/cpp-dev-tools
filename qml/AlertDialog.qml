import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import cdt
import "." as Cdt

Dialog {
  id: dialog
  property string dialogTitle: ""
  property string dialogText: ""
  property bool isError: false
  property bool isCancellable: true
  property bool isFullHeight: false
  onAccepted: viewSystem.alertDialogAccepted()
  onRejected: viewSystem.alertDialogRejected()
  Connections {
      target: viewSystem
      function onAlertDialogDisplayed(config) {
          dialogTitle = config.title;
          dialogText = config.text;
          isError = config.isError;
          isCancellable = config.isCancellable;
          isFullHeight = config.isFullHeight;
          dialog.visible = true;
      }
      function onDialogClosed() {
        dialog.reject();
      }
  }
  width: Theme.centeredViewWidth
  height: isFullHeight ? parent.height * 0.8 : Math.min(contentHeight + padding * 2, parent.height * 0.8)
  padding: Theme.basePadding * 2
  modal: true
  visible: false
  anchors.centerIn: parent
  background: Rectangle {
    color: Theme.colorBackground
    radius: Theme.baseRadius
    border.width: 1
    border.color: Theme.colorBorder
  }
  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 1
    Cdt.Text {
      text: dialogTitle
      font.bold: true
    }
    RowLayout {
      Layout.fillHeight: true
      Cdt.Icon {
        icon: dialog.isError ? "error" : "help"
        iconSize: 48
        color: dialog.isError ? "red" : Theme.colorText
        Layout.alignment: Qt.AlignTop
      }
      Cdt.BigTextArea {
        Layout.fillWidth: true
        Layout.fillHeight: true
        text: dialogText
        monoFont: false
        KeyNavigation.down: dialogCancel.visible ? dialogCancel : dialogOk
      }
    }
    Row {
      spacing: Theme.basePadding
      Layout.alignment: Qt.AlignRight
      Cdt.Button {
        id: dialogCancel
        text: "Cancel"
        focus: visible
        visible: dialog.isCancellable
        KeyNavigation.right: dialogOk
        onClicked: dialog.reject()
      }
      Cdt.Button {
        id: dialogOk
        text: "OK"
        focus: !dialogCancel.visible
        onClicked: dialog.accept()
      }
    }
  }
}
