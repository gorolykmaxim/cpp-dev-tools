import QtQuick
import QtQuick.Layouts
import cdt
import "." as Cdt

ColumnLayout {
  anchors.fill: parent
  GitLogModel {
    id: logModel
  }
  Cdt.TextList {
    Layout.fillWidth: true
    Layout.fillHeight: true
    focus: true
    model: logModel
  }
}
