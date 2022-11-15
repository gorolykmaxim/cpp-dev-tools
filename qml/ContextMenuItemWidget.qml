import QtQuick
import QtQuick.Controls

MenuItem {
  id: menuItem
  verticalPadding: basePadding
  horizontalPadding: basePadding * 2
  contentItem: TextWidget {
    text: parent.text
    color: menuItem.hovered ? colorBgBlack : colorText
  }
  background: Rectangle {
    color: menuItem.hovered ? colorHighlight : "transparent"
    radius: baseRadius
  }
}
