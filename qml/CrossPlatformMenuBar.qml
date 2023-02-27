import QtQuick
import QtQuick.Controls as Qml
import QtQuick.Layouts
import "Common.js" as Common
import cdt
import "." as Cdt

Qml.MenuBar {
  property var project
  property var model
  id: menuBar

  function initialize() {
    while (menuBar.count > 0) {
      menuBar.removeMenu(menuBar.menuAt(0));
    }
    if (project.isNull) {
      return;
    }
    for (let i = 0; i < model.rowCount(); i++) {
      const menuTitle = model.GetFieldByRoleName(i, "group");
      let menuObj = Common.findMenuByTitle(menuBar, menuTitle);
      if (menuObj === null) {
        menuObj = menu.createObject(menuBar, {title: menuTitle});
        menuBar.addMenu(menuObj);
      }
      const menuItemObj = action.createObject(menuObj, {
        text: model.GetFieldByRoleName(i, "name"),
        shortcut: model.GetFieldByRoleName(i, "shortcut"),
        callback: model.GetFieldByRoleName(i, "callback"),
      });
      menuObj.addAction(menuItemObj);
    }
  }

  onProjectChanged: initialize()
  onModelChanged: initialize()
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
      background: Rectangle {
        implicitWidth: 200
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
      property var callback
      onTriggered: callback()
    }
  }
}
