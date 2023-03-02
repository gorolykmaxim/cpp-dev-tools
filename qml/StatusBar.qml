import QtQuick
import QtQuick.Layouts
import cdt
import "." as Cdt

RowLayout {
  Row {
    padding: Theme.basePadding
    spacing: Theme.basePadding * 2
    Cdt.Text {
      text: projectContext.currentProjectShortPath || " "
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
