import QtQuick
import Qt.labs.platform
import "Common.js" as Common

Item {
  property var model: hActions

  onModelChanged: {
    menuBar.clear();
    if (model == null) {
      return;
    }
    for (let i = 0; i < model.rowCount(); i++) {
      const menuTitle = model.GetFieldByRole(i, 0);
      let menuObj = Common.findMenuByTitle(menuBar, menuTitle);
      if (menuObj === null) {
        menuObj = menu.createObject(menuBar, {title: menuTitle});
        menuBar.addMenu(menuObj);
      }
      const menuItemObj = menuItem.createObject(menuObj, {
        text: model.GetFieldByRole(i, 1),
        shortcut: model.GetFieldByRole(i, 2),
        eventType: model.GetFieldByRole(i, 3),
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
