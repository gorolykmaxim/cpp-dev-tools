import QtQuick
import QtQuick.Layouts

RowLayout {
  Row {
    padding: basePadding
    spacing: basePadding * 2
    Repeater {
      model: sItemsLeft
      TextWidget {
        text: title
      }
    }
  }
  Row {
    padding: basePadding
    spacing: basePadding * 2
    Layout.alignment: Qt.AlignRight
    Repeater {
      model: sItemsRight
      TextWidget {
        text: title
      }
    }
  }
}
