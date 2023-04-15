import QtQuick
import QtQuick.Layouts
import cdt
import "." as Cdt

RowLayout {
  Row {
    padding: Theme.basePadding
    spacing: Theme.basePadding * 2
    Cdt.Text {
      text: projectSystem.currentProjectShortPath || " "
    }
  }
  Row {
    padding: Theme.basePadding
    spacing: Theme.basePadding * 2
    Layout.alignment: Qt.AlignRight
    Cdt.Text {
      color: notificationSystem.notSeenNotifications > 0 ?
               (notificationSystem.newErrors ? "red" : Theme.colorText) :
               Theme.colorSubText
      text: notificationSystem.notSeenNotifications > 0 ?
              "Notifications: " + notificationSystem.notSeenNotifications :
              "No Notifications"
    }
    Cdt.Text {
      text: taskSystem.currentTaskName ? "[" + taskSystem.currentTaskName + "]" : ""
    }
  }
}
