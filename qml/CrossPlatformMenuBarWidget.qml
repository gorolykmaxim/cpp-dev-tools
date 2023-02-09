import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Common.js" as Common

MenuBar {
  property var model: hActions
  id: menuBar

  onModelChanged: {
    for (let i = 0; i < menuBar.count; i++) {
      const menuObj = menuBar.menuAt(i);
      menuBar.removeMenu(menuObj);
    }
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
      const menuItemObj = action.createObject(menuObj, {
        text: model.GetFieldByRole(i, 1),
        shortcut: model.GetFieldByRole(i, 2),
        eventType: model.GetFieldByRole(i, 3),
      });
      menuObj.addAction(menuItemObj);
    }
  }

  background: Rectangle {
    implicitHeight: 1
    color: colorBgDark
  }
  delegate: MenuBarItem {
    padding: basePadding
    contentItem: TextWidget {
      highlight: parent.highlighted
      text: parent.text
    }
    background: Rectangle {
      implicitWidth: 1
      implicitHeight: 1
      color: parent.highlighted ? colorBgMedium : colorBgDark
    }
  }

  Component {
    id: menu
    Menu {
      background: Rectangle {
        implicitWidth: 200
        implicitHeight: 1
        border.width: 1
        border.color: colorBgMedium
        color: colorBgDark
      }
      delegate: MenuItem {
        horizontalPadding: basePadding * 3
        contentItem: RowLayout {
          TextWidget {
            Layout.alignment: Qt.AlignLeft
            text: parent.parent.text
            highlight: parent.parent.highlighted
          }
          TextWidget {
            Layout.alignment: Qt.AlignRight
            text: parent.parent.action.shortcut
            color: colorSubText
          }
        }
        background: Rectangle {
          color: parent.highlighted ? colorBgMedium : "transparent"
        }
      }
    }
  }

  Component {
    id: action
    Action {
      property var eventType
      onTriggered: core.OnAction(eventType, [])
    }
  }
}
