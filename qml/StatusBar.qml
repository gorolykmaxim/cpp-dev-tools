import QtQuick
import QtQuick.Layouts
import cdt
import "." as Cdt

RowLayout {
  readonly property int interItemSpacing: Theme.basePadding * 4
  Row {
    padding: Theme.basePadding
    spacing: interItemSpacing
    Cdt.StatusBarItem {
      text: projectSystem.currentProjectShortPath || " "
    }
  }
  Row {
    padding: Theme.basePadding
    spacing: 0
    Layout.alignment: Qt.AlignRight
    Cdt.Text {
      width: Math.min(implicitWidth, 300)
      text: notificationSystem.lastNotSeenNotificationTitle
      color: notificationsItem.iconAndTextColor
      elide: Text.ElideRight
      rightPadding: Theme.basePadding
    }
    Cdt.StatusBarItem {
      id: notificationsItem
      property string iconAndTextColor: notificationSystem.notSeenNotifications > 0 ?
                                          (notificationSystem.newErrors ? "red" : Theme.colorText) :
                                          Theme.colorSubText
      displayIcon: projectSystem.currentProjectShortPath
      iconName: notificationSystem.notSeenNotifications > 0 ? "notifications" : "notifications_none"
      iconColor: iconAndTextColor
      text: notificationSystem.notSeenNotifications > 0 ? notificationSystem.notSeenNotifications : ""
      textColor: iconAndTextColor
      rightPadding: interItemSpacing
    }
    Cdt.StatusBarItem {
      displayIcon: sqliteSystem.selectedFileName
      iconName: "storage"
      text: sqliteSystem.selectedFileName || ""
      rightPadding: interItemSpacing
    }
    Cdt.StatusBarItem {
      displayIcon: taskSystem.currentTaskName
      iconName: "code"
      text: taskSystem.currentTaskName || ""
    }
  }
}
