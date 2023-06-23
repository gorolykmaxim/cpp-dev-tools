import QtQuick
import QtQuick.Controls as QtQuick
import cdt
import "." as Cdt

QtQuick.Button {
  property string buttonIcon: ""
  padding: Theme.basePadding
  background: Rectangle {
    color: "transparent"
  }
  contentItem: Cdt.Icon {
    icon: buttonIcon
    color: !parent.enabled ?
             Theme.colorPlaceholder :
             (parent.activeFocus || checked ?
                (pressed || (checked && parent.activeFocus) ? "#43b7ff" : Theme.colorPrimary) :
                (pressed ? Theme.colorText : Theme.colorPlaceholder))
  }
  Keys.onReturnPressed: checkable ? toggle() : clicked()
  Keys.onEnterPressed: checkable ? toggle() : clicked()
}
