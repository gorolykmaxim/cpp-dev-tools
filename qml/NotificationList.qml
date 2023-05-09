import QtQuick
import QtQuick.Layouts
import Qt.labs.platform
import "." as Cdt
import cdt

Loader {
  id: root
  focus: true
  anchors.fill: parent
  sourceComponent: listView
  NotificationListController {
    id: controller
  }
  Component {
    id: listView
    Cdt.SearchableTextList {
      id: list
      Component.onCompleted: controller.displayList()
      anchors.fill: parent
      searchPlaceholderText: "Search notification"
      showPlaceholder: controller.areNotificationsEmpty
      placeholderText: "No Notifications Found"
      searchableModel: controller.notifications
      focus: true
      onCurrentItemChanged: ifCurrentItem('idx', idx => controller.selected = idx)
      onItemSelected: root.sourceComponent = detailsView
      onItemRightClicked: contextMenu.open()
      Menu {
        id: contextMenu
        MenuItem {
          text: "Clear Notifications"
          enabled: list.activeFocus
          shortcut: "Alt+Shift+D"
          onTriggered: controller.clearNotifications()
        }
      }
    }
  }
  Component {
    id: detailsView
    ColumnLayout {
      anchors.fill: parent
      spacing: 0
      Keys.onEscapePressed: backBtn.clicked()
      Cdt.Pane {
        Layout.fillWidth: true
        color: Theme.colorBgMedium
        padding: Theme.basePadding
        RowLayout {
          anchors.fill: parent
          Cdt.Icon {
            icon: controller.selectedNotificationIcon
            color: controller.selectedNotificationIconColor || Theme.colorText
            Layout.margins: Theme.basePadding
          }
          Cdt.Text {
            Layout.fillWidth: true
            elide: Text.ElideLeft
            text: controller.selectedNotificationTitle
            color: controller.selectedNotificationTitleColor
          }
          Cdt.Button {
            id: backBtn
            text: "Back"
            onClicked: root.sourceComponent = listView
          }
        }
      }
      Rectangle {
        Layout.fillWidth: true
        height: 1
        color: Theme.colorBgBlack
      }
      Cdt.TextArea {
        Layout.fillWidth: true
        Layout.fillHeight: true
        focus: true
        readonly: true
        innerPadding: Theme.basePadding
        color: Theme.colorBgDark
        text: controller.selectedNotificationDescription
        searchable: true
        KeyNavigation.up: backBtn
      }
    }
  }
}

