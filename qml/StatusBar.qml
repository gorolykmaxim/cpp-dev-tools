import QtQuick
import QtQuick.Layouts
import cdt
import "." as Cdt

ColumnLayout {
  readonly property int interItemSpacing: Theme.basePadding * 4
  spacing: 0
  Rectangle {
    Layout.fillWidth: true
    height: 1
    color: Theme.colorBorder
  }
  RowLayout {
    Layout.fillWidth: true
    Row {
      Layout.fillWidth: true
      padding: Theme.basePadding
      spacing: interItemSpacing
      Cdt.StatusBarItem {
        text: projectSystem.currentProjectShortPath || " "
      }
    }
    Row {
      padding: Theme.basePadding
      spacing: 0
      Cdt.Text {
        width: Math.min(implicitWidth, 300)
        text: notificationSystem.lastNotSeenNotificationTitle
        color: notificationSystem.isLastNotSeenNotificationError ? "red" : Theme.colorText
        elide: Text.ElideRight
        rightPadding: Theme.basePadding
      }
      Cdt.StatusBarItem {
        property string iconAndTextColor: notificationSystem.notSeenNotifications > 0 ?
                                            (notificationSystem.newErrors ? "red" : Theme.colorText) :
                                            Theme.colorPlaceholder
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
        rightPadding: interItemSpacing
      }
      Cdt.StatusBarItem {
        displayIcon: gitSystem.currentBranch
        iconName: "call_split"
        text: gitSystem.currentBranch || ""
      }
    }
  }
}
