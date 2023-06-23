import QtQuick
import QtQuick.Layouts
import "." as Cdt
import cdt

Cdt.Pane {
  id: root
  required property int itemIndex
  required property int itemOffset
  required property int lineNumberMaxWidth
  required property string text
  property var lineNumber: null
  property list<QtObject> formatters: []
  property bool displayLineNumber: true
  property bool monoFont: true
  property string lineColor: "transparent"
  signal inlineSelect(int line, int startOffset, int endOffset)
  signal pressed()
  signal rightClick()
  signal ctrlLeftClick()
  function rehighlightIfInRange(first, last) {
    if (itemIndex >= first && itemIndex <= last) {
      highlighter.rehighlight();
    }
  }
  RowLayout {
    anchors.fill: parent
    spacing: 0
    Cdt.Text {
      visible: root.displayLineNumber
      text: lineNumber !== null ? (lineNumber > 0 ? lineNumber : '') : itemIndex + 1
      color: Theme.colorPlaceholder
      Layout.fillHeight: true
      Layout.minimumWidth: root.lineNumberMaxWidth
      Layout.leftMargin: Theme.basePadding
      horizontalAlignment: Text.AlignRight
      verticalAlignment: Text.AlignTop
      font.family: root.monoFont ? monoFontFamily : null
      font.pointSize: root.monoFont ? monoFontSize : -1
    }
    Cdt.Pane {
      Layout.leftMargin: Theme.basePadding
      Layout.rightMargin: Theme.basePadding
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: root.lineColor
      TextEdit {
        id: textEdit
        property bool ignoreSelect: false
        anchors.fill: parent
        activeFocusOnPress: false
        selectByMouse: true
        selectByKeyboard: true
        readOnly: true
        enabled: root.enabled
        selectionColor: "transparent"
        selectedTextColor: "transparent"
        text: root.text
        textFormat: TextEdit.PlainText
        color: root.enabled ? Theme.colorText : Theme.colorPlaceholder
        font.family: root.monoFont ? monoFontFamily : Qt.application.font.family
        font.pointSize: root.monoFont ? monoFontSize : Qt.application.font.pointSize
        renderType: Text.NativeRendering
        wrapMode: Text.WordWrap
        onSelectedTextChanged: {
          if (ignoreSelect) {
            return;
          }
          ignoreSelect = true;
          inlineSelect(itemIndex, selectionStart, selectionEnd);
          ignoreSelect = false;
        }
        LineHighlighter {
          id: highlighter
          document: textEdit.textDocument
          formatters: root.formatters
          lineNumber: itemIndex
          lineOffset: itemOffset
        }
        MouseArea {
          anchors.fill: parent
          acceptedButtons: Qt.LeftButton | Qt.RightButton
          cursorShape: Qt.IBeamCursor
          onPressed: function(event) {
            root.pressed();
            if (event.button === Qt.RightButton) {
              root.rightClick();
              event.accepted = true;
            } else if (event.modifiers & Qt.ControlModifier) {
              root.ctrlLeftClick();
              event.accepted = true;
            } else {
              event.accepted = false;
            }
          }
        }
      }
    }
  }
}
