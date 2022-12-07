import QtQuick
import Qt.labs.platform

MenuBar {
  id: menuBar
  property var model

  function getValue(row, col) {
    return model.data(model.index(row, 0), col);
  }

  function findMenuByTitle(menuTitle) {
    for (let i = 0; i < menuBar.menus.length; i++) {
      const menuObj = menuBar.menus[i];
      if (menuObj.title === menuTitle) {
        return menuObj;
      }
    }
    return null;
  }

  onModelChanged: {
    menuBar.clear();
    if (model == null) {
      return;
    }
    for (let i = 0; i < model.rowCount(); i++) {
      const menuTitle = getValue(i, 0);
      let menuObj = findMenuByTitle(menuTitle);
      if (menuObj === null) {
        menuObj = menu.createObject(menuBar, {title: menuTitle});
        menuBar.addMenu(menuObj);
      }
      const menuItemObj = menuItem.createObject(menuObj, {
        text: getValue(i, 1),
        shortcut: getValue(i, 2),
        eventType: getValue(i, 3),
      });
      menuObj.addItem(menuItemObj);
    }
  }

  Component {
    id: menu
    Menu {
    }
  }

  Component {
    id: menuItem

    MenuItem {
      property var eventType
      onTriggered: core.OnAction(eventType, [])
    }
  }
}
