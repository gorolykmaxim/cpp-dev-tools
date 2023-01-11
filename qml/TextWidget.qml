import QtQuick.Controls

Label {
  property var textColor: colorText
  property var highlight: false
  color: highlight ? colorHighlight : textColor
}
