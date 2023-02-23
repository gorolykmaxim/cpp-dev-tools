import QtQuick.Controls
import cdt

Label {
  property var textColor: Theme.colorText
  property var highlight: false
  color: highlight ? Theme.colorHighlight : textColor
}
