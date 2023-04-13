import QtQuick.Controls
import cdt

Label {
  property string textColor: Theme.colorText
  property bool highlight: false
  color: highlight ? Theme.colorHighlight : textColor
}
