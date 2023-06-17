import QtQuick
import Qt.labs.platform

Menu {
  id: root
  property var itemsList: []
  required property var parentView
  function redraw() {
    root.clear();
    itemsList.forEach(i => root.addItem(menuItemComponent.createObject(root, i)));
  }
  Component.onCompleted: redraw()
  onItemsListChanged: redraw()
  Component {
    id: menuItemComponent
    MenuItem {
      property var callback: () => {}
      enabled: parentView?.activeFocus
      onTriggered: callback()
    }
  }
}
