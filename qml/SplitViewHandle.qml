import QtQuick
import cdt

Rectangle {
  required property string viewId
  required property var view
  Component.onCompleted: view.restoreState(viewSystem.getSplitViewState(viewId))
  Connections {
    target: view
    function onResizingChanged() {
      if (!view.resizing) {
        viewSystem.saveSplitViewState(viewId, view.saveState());
      }
    }
  }
  implicitWidth: Theme.basePadding
  implicitHeight: Theme.basePadding
  color: parent.resizing ? Theme.colorHighlight : Theme.colorBgBlack
}
