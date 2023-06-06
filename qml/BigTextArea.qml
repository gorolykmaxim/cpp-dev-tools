import QtQuick
import QtQuick.Controls
import "." as Cdt
import cdt

Cdt.Pane {
  id: root
  property alias text: controller.text
  property bool monoFont: true
  property alias cursorFollowEnd: controller.cursorFollowEnd
  color: Theme.colorBgDark
  BigTextAreaController {
    id: controller
    onGoToLine: function(line) {
      listView.currentIndex = line;
    }
  }
  ListView {
    id: listView
    anchors.fill: parent
    focus: true
    clip: true
    boundsBehavior: ListView.StopAtBounds
    boundsMovement: ListView.StopAtBounds
    highlightMoveDuration: 100
    ScrollBar.vertical: ScrollBar {}
    model: controller.textModel
    delegate: TextEdit {
      width: listView.width
      selectByMouse: true
      readOnly: true
      enabled: root.enabled
      selectionColor: Theme.colorHighlight
      text: model.text
      textFormat: TextEdit.PlainText
      color: root.enabled ? Theme.colorText : Theme.colorSubText
      font.family: root.monoFont ? monoFontFamily : null
      font.pointSize: root.monoFont ? monoFontSize : -1
      renderType: Text.NativeRendering
      wrapMode: Text.WordWrap
      MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: listView.currentIndex = index
      }
    }
  }
}
