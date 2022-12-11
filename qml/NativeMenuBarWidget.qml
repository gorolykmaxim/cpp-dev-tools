import QtQuick
import Qt.labs.platform
import "Common.js" as Common

Item {
  property var model

  onModelChanged: {
    menuBar.clear();
    if (model == null) {
      return;
    }
    for (let i = 0; i < model.rowCount(); i++) {
      const menuTitle = Common.getListModelValue(model, i, 0);
      let menuObj = Common.findMenuByTitle(menuBar, menuTitle);
      if (menuObj === null) {
        menuObj = menu.createObject(menuBar, {title: menuTitle});
        menuBar.addMenu(menuObj);
      }
      const menuItemObj = menuItem.createObject(menuObj, {
        text: Common.getListModelValue(model, i, 1),
        shortcut: Common.getListModelValue(model, i, 2),
        eventType: Common.getListModelValue(model, i, 3),
      });
      menuObj.addItem(menuItemObj);
    }
  }
  MenuBar {
    id: menuBar

    Component {
      id: menu
      Menu {}
    }

    Component {
      id: menuItem

      MenuItem {
        property var eventType
        onTriggered: core.OnAction(eventType, [])
      }
    }
  }
}
