#include "notification_list_controller.h"

#include "application.h"
#include "theme.h"
#include "ui_icon.h"

NotificationListController::NotificationListController(QObject *parent)
    : QObject(parent), notifications(new NotificationListModel(this)) {
  Application &app = Application::Get();
  app.view.SetWindowTitle("Notifications");
  notifications->last_seen_notification =
      app.notification.IndexOfLastSeenNotification();
  QObject::connect(&app.notification, &NotificationSystem::notificationsChanged,
                   this, [this] {
                     notifications->Load();
                     emit notificationsChanged();
                   });
}

bool NotificationListController::AreNotificationsEmpty() const {
  return notifications->GetRowCount() == 0;
}

QString NotificationListController::GetSelectedNotificationIcon() const {
  if (selected < 0) {
    return "";
  }
  return NotificationListModel::GetIconOf(notifications->At(selected)).icon;
}

QString NotificationListController::GetSelectedNotificationIconColor() const {
  if (selected < 0) {
    return "";
  }
  return NotificationListModel::GetIconOf(notifications->At(selected)).color;
}

QString NotificationListController::GetSelectedNotificationTitle() const {
  if (selected < 0) {
    return "";
  }
  return notifications->At(selected).title;
}

QString NotificationListController::GetSelectedNotificationTitleColor() const {
  if (selected < 0) {
    return "";
  }
  return notifications->GetTitleColorOf(selected);
}

QString NotificationListController::GetSelectedNotificationDescription() const {
  if (selected < 0) {
    return "";
  }
  return notifications->At(selected).description;
}

void NotificationListController::displayList() {
  Application::Get().notification.MarkAllNotificationsSeenByUser();
}

void NotificationListController::clearNotifications() {
  Application &app = Application::Get();
  app.notification.ClearNotifications();
  notifications->last_seen_notification =
      app.notification.IndexOfLastSeenNotification();
}

UiIcon NotificationListModel::GetIconOf(const Notification &notification) {
  UiIcon icon;
  if (notification.is_error) {
    icon.icon = "error";
    icon.color = "red";
  } else {
    icon.icon = "info";
  }
  return icon;
}

NotificationListModel::NotificationListModel(QObject *parent)
    : QVariantListModel(parent) {
  SetRoleNames({{0, "idx"},
                {1, "title"},
                {2, "titleColor"},
                {3, "icon"},
                {4, "iconColor"},
                {5, "isSelected"}});
  searchable_roles = {0, 1};
}

QVariantList NotificationListModel::GetRow(int i) const {
  const Notification &notification = At(i);
  UiIcon icon = GetIconOf(notification);
  QString title_color = GetTitleColorOf(i);
  bool is_selected = i == GetRowCount() - 1;
  return {i,         notification.title, title_color,
          icon.icon, icon.color,         is_selected};
}

int NotificationListModel::GetRowCount() const {
  return Application::Get().notification.GetNotifications().size();
}

const Notification &NotificationListModel::At(int i) const {
  return Application::Get().notification.GetNotifications()[i];
}

QString NotificationListModel::GetTitleColorOf(int i) const {
  static const Theme kTheme;
  if (i <= last_seen_notification) {
    return kTheme.kColorSubText;
  } else {
    return kTheme.kColorText;
  }
}
