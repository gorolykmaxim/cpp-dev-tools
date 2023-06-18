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
  property list<QtObject> formatters: []
  property bool displayLineNumber: true
  property bool monoFont: true
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
      text: itemIndex + 1
      color: Theme.colorSubText
      Layout.fillHeight: true
      Layout.minimumWidth: root.lineNumberMaxWidth
      Layout.leftMargin: Theme.basePadding
      horizontalAlignment: Text.AlignRight
      verticalAlignment: Text.AlignTop
      font.family: root.monoFont ? monoFontFamily : null
      font.pointSize: root.monoFont ? monoFontSize : -1
    }
    TextEdit {
      id: textEdit
      property bool ignoreSelect: false
      Layout.leftMargin: Theme.basePadding
      Layout.rightMargin: Theme.basePadding
      Layout.fillWidth: true
      activeFocusOnPress: false
      selectByMouse: true
      selectByKeyboard: true
      readOnly: true
      enabled: root.enabled
      selectionColor: "transparent"
      selectedTextColor: "transparent"
      text: root.text
      textFormat: TextEdit.PlainText
      color: root.enabled ? Theme.colorText : Theme.colorSubText
      font.family: root.monoFont ? monoFontFamily : null
      font.pointSize: root.monoFont ? monoFontSize : -1
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
