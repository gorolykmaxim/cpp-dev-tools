import QtQuick
import QtQuick.Controls as QtQuick
import cdt
import "." as Cdt

QtQuick.Button {
  topPadding: Theme.basePadding
  bottomPadding: Theme.basePadding
  leftPadding: Theme.basePadding * 4
  rightPadding: Theme.basePadding * 4
  background: Rectangle {
    color: !enabled ? Theme.colorBgDimmed : (hovered && !pressed ? Theme.colorBgBright : Theme.colorBgLight)
    border.color: !enabled ? Theme.colorBgMedium : (parent.activeFocus ? Theme.colorHighlight : Theme.colorBgDark)
    border.width: parent.activeFocus ? 2 : 1
    radius: Theme.baseRadius
  }
  contentItem: Cdt.Text {
    text: parent.text
    horizontalAlignment: Text.AlignHCenter
    color: parent.enabled ? Theme.colorText : Theme.colorSubText
  }
  Keys.onReturnPressed: clicked()
  Keys.onEnterPressed: clicked()
}
