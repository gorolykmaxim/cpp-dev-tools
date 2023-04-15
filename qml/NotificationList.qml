import QtQuick
import Qt.labs.platform
import "." as Cdt
import cdt

Cdt.SearchableTextList {
  id: list
  anchors.fill: parent
  searchPlaceholderText: "Search notification"
  showPlaceholder: controller.areNotificationsEmpty
  placeholderText: "No Notifications Found"
  searchableModel: controller.notifications
  focus: true
  NotificationListController {
    id: controller
  }
}
