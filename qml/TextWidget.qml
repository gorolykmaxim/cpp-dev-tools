import QtQuick.Controls

Label {
  property var highlight: false
  color: highlight ? colorHighlight : colorText
}
