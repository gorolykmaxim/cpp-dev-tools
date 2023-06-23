import QtQuick
import QtQuick.Controls as QtQuick
import cdt
import "." as Cdt

QtQuick.Button {
  id: btn
  readonly property string defaultColor: "#616161"
  readonly property string defaultClickedColor: "#898888"
  topPadding: Theme.basePadding
  bottomPadding: Theme.basePadding
  leftPadding: Theme.basePadding * 4
  rightPadding: Theme.basePadding * 4
  background: Rectangle {
    gradient: Gradient {
      GradientStop { position: 0.0; color: btn.activeFocus ? (pressed ? Theme.colorPrimary : Theme.colorButtonPrimaryLight) : (pressed ? defaultClickedColor : defaultColor) }
      GradientStop { position: 1.0; color: btn.activeFocus ? (pressed ? Theme.colorPrimary : Theme.colorButtonPrimaryDark) : (pressed ? defaultClickedColor : defaultColor) }
    }
    border.width: 1
    border.color: parent.activeFocus ? Theme.colorButtonPrimaryDark : Theme.colorButtonBorder
    radius: Theme.baseRadius
  }
  contentItem: Cdt.Text {
    text: parent.text
    horizontalAlignment: Text.AlignHCenter
    color: parent.enabled ? Theme.colorText : Theme.colorPlaceholder
  }
  Keys.onReturnPressed: clicked()
  Keys.onEnterPressed: clicked()
}
