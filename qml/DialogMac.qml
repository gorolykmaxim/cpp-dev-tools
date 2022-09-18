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
    ScrollView {
      Layout.fillWidth: true
      Layout.maximumHeight: 300
      TextArea {
        background: Rectangle {
          color: "transparent"
        }
        text: dialogDataText ?? ""
        wrapMode: TextArea.WordWrap
        KeyNavigation.down: dialogCancel.visible ? dialogCancel : dialogOk
        // Make text area effectively readOnly but don't hide the cursor and
        // allow navigating it using the cursor
        Keys.onPressed: event => {
          if (event.key != Qt.Key_Left && event.key != Qt.Key_Right &&
              event.key != Qt.Key_Up && event.key != Qt.Key_Down) {
            event.accepted = true;
          }
        }
      }
    }
    Row {
      Layout.alignment: Qt.AlignRight
      Button {
        id: dialogCancel
        text: "Cancel"
        focus: visible
        visible: dialogDataCancellable ?? false
        Keys.onReturnPressed: clicked()
        Keys.onEnterPressed: clicked()
        KeyNavigation.right: dialogOk
        onClicked: {
          dialog.reject();
          core.OnDialogResult(false);
        }
      }
      Button {
        id: dialogOk
        text: "OK"
        focus: !dialogCancel.visible
        highlighted: true
        Keys.onReturnPressed: clicked()
        Keys.onEnterPressed: clicked()
        onClicked: {
          dialog.accept();
          core.OnDialogResult(true);
        }
      }
    }
  }
}
