import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
  id: dialog
  title: dialogDataTitle
  width: parent.width / 2
  modal: true
  visible: dialogDataVisible
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
        text: dialogDataText
        wrapMode: TextArea.WordWrap
        KeyNavigation.down: dialogButton
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
    Button {
      id: dialogButton
      text: dialogDataButtonText
      focus: true
      highlighted: true
      Layout.alignment: Qt.AlignRight
      Keys.onReturnPressed: clicked()
      Keys.onEnterPressed: clicked()
      onClicked: dialog.accept()
    }
  }
}
