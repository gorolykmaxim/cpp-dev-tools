import QtQuick
import Qt.labs.platform
import "Common.js" as Common

Item {
  property bool isProjectOpened: false
  property var model

  function initialize() {
    menuBar.clear();
    if (!isProjectOpened) {
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

  onIsProjectOpenedChanged: initialize()
  onModelChanged: initialize()
  Timer {
    id: delay
    property var callback
    interval: 0
    onTriggered: callback()
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
        property var callback
        onTriggered: {
          // Calling callback synchronously from here can crash the app
          // if the callback causes this menubar to change it's structure
          // and delete current menu item.
          delay.callback = callback;
          delay.running = true;
        }
      }
    }
  }
}
