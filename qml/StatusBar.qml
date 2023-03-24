import QtQuick
import QtQuick.Layouts
import cdt
import "." as Cdt

RowLayout {
  Row {
    padding: Theme.basePadding
    spacing: Theme.basePadding * 2
    Cdt.Text {
      text: projectSystem.currentProjectShortPath || " "
    }
  }
  Row {
    padding: Theme.basePadding
    spacing: Theme.basePadding * 2
    Layout.alignment: Qt.AlignRight
    Cdt.Text {
      text: taskSystem.currentTaskName ? "[" + taskSystem.currentTaskName + "]" : ""
    }
  }
}
