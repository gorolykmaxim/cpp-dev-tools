import QtQuick
import QtQuick.Controls

TextField {
  color: colorText
  background: Rectangle {
    color: colorBgDark
    border.color: parent.focus ? colorHighlight : colorBgBlack
    border.width: parent.focus ? 2 : 1
    radius: baseRadius
  }
}
