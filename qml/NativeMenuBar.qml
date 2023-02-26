import QtQuick
import Qt.labs.platform
import "Common.js" as Common

Item {
  property var project: null
  property var model: null

  function initialize() {
    menuBar.clear();
    if (model == null || project == null) {
      return;
    }
    for (let i = 0; i < model.rowCount(); i++) {
      const menuTitle = model.GetFieldByRoleName(i, "group");
      let menuObj = Common.findMenuByTitle(menuBar, menuTitle);
      if (menuObj === null) {
        menuObj = menu.createObject(menuBar, {title: menuTitle});
        menuBar.addMenu(menuObj);
      }
      const menuItemObj = menuItem.createObject(menuObj, {
        text: model.GetFieldByRoleName(i, "name"),
        shortcut: model.GetFieldByRoleName(i, "shortcut"),
        callback: model.GetFieldByRoleName(i, "callback"),
      });
      menuObj.addItem(menuItemObj);
    }
  }

  onProjectChanged: initialize()
  onModelChanged: initialize()
  MenuBar {
    id: menuBar

    Component {
      id: menu
      Menu {}
    }

    Component {
      id: menuItem

      MenuItem {
        property var callback
        onTriggered: callback()
      }
    }
  }
}
