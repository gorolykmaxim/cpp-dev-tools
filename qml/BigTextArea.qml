import QtQuick
import QtQuick.Controls
import "." as Cdt
import cdt

Cdt.Pane {
  id: root
  property alias text: controller.text
  property bool monoFont: true
  color: Theme.colorBgDark
  BigTextAreaController {
    id: controller
  }
  ListView {
    id: listView
    property var allowedReadonlyKeys: new Set([
      Qt.Key_Left,
      Qt.Key_Right,
      Qt.Key_Up,
      Qt.Key_Down,
      Qt.Key_Home,
      Qt.Key_End,
      Qt.Key_Escape,
      Qt.Key_F5,
    ])
    anchors.fill: parent
    focus: true
    clip: true
    boundsBehavior: ListView.StopAtBounds
    boundsMovement: ListView.StopAtBounds
    highlightMoveDuration: 100
    ScrollBar.vertical: ScrollBar {}
    model: controller.textModel
    delegate: TextEdit {
      property var itemModel: model
      property var itemIndex: index
      width: listView.width
      selectByMouse: true
      enabled: root.enabled
      selectionColor: Theme.colorHighlight
      text: itemModel.text
      textFormat: TextEdit.PlainText
      color: root.enabled ? Theme.colorText : Theme.colorSubText
      font.family: root.monoFont ? monoFontFamily : null
      font.pointSize: root.monoFont ? monoFontSize : -1
      renderType: Text.NativeRendering
      wrapMode: Text.WordWrap
      onTextChanged: {
        if (text !== itemModel.text) {
          text = itemModel.text;
        }
      }
      Keys.onPressed: function(event) {
        if (!listView.allowedReadonlyKeys.has(event.key) && !event.matches(StandardKey.Copy) &&
            !event.matches(StandardKey.SelectAll)) {
          event.accepted = true;
        }
      }
    }
  }
}
