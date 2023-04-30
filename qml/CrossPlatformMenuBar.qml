import QtQuick
import QtQuick.Controls as Qml
import QtQuick.Layouts
import "Common.js" as Common
import cdt
import "." as Cdt

Qml.MenuBar {
  property bool isProjectOpened: false
  property var model
  id: menuBar

  function initialize() {
    while (menuBar.count > 0) {
      menuBar.removeMenu(menuBar.menuAt(0));
    }
    if (!isProjectOpened) {
      return;
    }
    const groups = [];
    const groupNameToMenuItems = {};
    const groupNameToMaxItemWidth = {};
    for (let i = 0; i < model.rowCount(); i++) {
      const groupName = model.getFieldByRoleName(i, "group");
      const text = model.getFieldByRoleName(i, "name");
      const shortcut = model.getFieldByRoleName(i, "shortcut");
      const itemWidth = 7 * text.length + 9 * shortcut.length;
      const index = model.getFieldByRoleName(i, "index");
      if (groups.indexOf(groupName) < 0) {
        groups.push(groupName);
      }
      let maxItemWidth = groupNameToMaxItemWidth[groupName] || 0;
      if (itemWidth > maxItemWidth) {
        groupNameToMaxItemWidth[groupName] = itemWidth;
      }
      let menuItems = groupNameToMenuItems[groupName];
      if (!menuItems) {
        menuItems = [];
        groupNameToMenuItems[groupName] = menuItems;
      }
      menuItems.push({
        text: text,
        shortcut: shortcut,
        index: index,
      });
    }
    for (let i = 0; i < groups.length; i++) {
      const groupName = groups[i];
      const maxItemWidth = groupNameToMaxItemWidth[groupName];
      const menuItems = groupNameToMenuItems[groupName];
      const menuObj = menu.createObject(menuBar, {title: groupName, maxItemWidth: maxItemWidth});
      for (let j = 0; j < menuItems.length; j++) {
        const menuItem = menuItems[j];
        const menuItemObj = action.createObject(menuObj, {
          text: menuItem.text,
          shortcut: menuItem.shortcut,
          index: menuItem.index,
        });
        menuObj.addAction(menuItemObj);
      }
      menuBar.addMenu(menuObj);
    }
  }

  onIsProjectOpenedChanged: initialize()
  Connections {
    target: model
    function onIsUpdatingChanged() {
      if (!model.isUpdating) {
        initialize();
      }
    }
  }
  background: Rectangle {
    implicitHeight: 1
    color: Theme.colorBgDark
  }
  delegate: Qml.MenuBarItem {
    padding: Theme.basePadding
    contentItem: Cdt.Text {
      highlight: parent.highlighted
      text: parent.text
    }
    background: Rectangle {
      implicitWidth: 1
      implicitHeight: 1
      color: parent.highlighted ? Theme.colorBgMedium : Theme.colorBgDark
    }
  }

  Component {
    id: menu
    Qml.Menu {
      id: menuRoot
      property real maxItemWidth: 0
      background: Rectangle {
        implicitWidth: menuRoot.maxItemWidth
        implicitHeight: 1
        border.width: 1
        border.color: Theme.colorBgMedium
        color: Theme.colorBgDark
      }
      delegate: Qml.MenuItem {
        horizontalPadding: Theme.basePadding * 3
        contentItem: RowLayout {
          Cdt.Text {
            Layout.alignment: Qt.AlignLeft
            text: parent.parent.text
            highlight: parent.parent.highlighted
          }
          Cdt.Text {
            Layout.alignment: Qt.AlignRight
            text: parent.parent.action.shortcut
            color: Theme.colorSubText
          }
        }
        background: Rectangle {
          color: parent.highlighted ? Theme.colorBgMedium : "transparent"
        }
      }
    }
  }

  Component {
    id: action
    Qml.Action {
      property var index
      onTriggered: userCommandSystem.executeCommand(index)
    }
  }
}
