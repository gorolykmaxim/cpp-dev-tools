import QtQuick
import QtQuick.Layouts
import cdt
import "." as Cdt

RowLayout {
  property var project: null
  Row {
    padding: Theme.basePadding
    spacing: Theme.basePadding * 2
    Cdt.Text {
      text: project ? project.pathRelativeToHome : " "
    }
  }
  Row {
    padding: Theme.basePadding
    spacing: Theme.basePadding * 2
    Layout.alignment: Qt.AlignRight
    Cdt.Text {
      text: ""
    }
  }
}
