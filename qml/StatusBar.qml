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
    spacing: interItemSpacing
    Layout.alignment: Qt.AlignRight
    Cdt.StatusBarItem {
      property string iconAndTextColor: notificationSystem.notSeenNotifications > 0 ?
                                          (notificationSystem.newErrors ? "red" : Theme.colorText) :
                                          Theme.colorSubText
      displayIcon: true
      iconName: notificationSystem.notSeenNotifications > 0 ? "notifications" : "notifications_none"
      iconColor: iconAndTextColor
      text: notificationSystem.notSeenNotifications > 0 ? notificationSystem.notSeenNotifications : ""
      textColor: iconAndTextColor
    }
    Cdt.StatusBarItem {
      displayIcon: sqliteSystem.selectedFileName
      iconName: "storage"
      text: sqliteSystem.selectedFileName || ""
    }
    Cdt.StatusBarItem {
      displayIcon: taskSystem.currentTaskName
      iconName: "code"
      text: taskSystem.currentTaskName || ""
    }
  }
}
