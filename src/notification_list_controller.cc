#include "notification_list_controller.h"

#include "application.h"
#include "theme.h"
#include "ui_icon.h"

NotificationListController::NotificationListController(QObject *parent)
    : QObject(parent), notifications(new NotificationListModel(this)) {
  Application &app = Application::Get();
  app.view.SetWindowTitle("Notifications");
  QObject::connect(&app.notification, &NotificationSystem::notificationsChanged,
                   this, [this] {
                     notifications->Load();
                     emit notificationsChanged();
                   });
  app.notification.MarkAllNotificationsSeenByUser();
}

bool NotificationListController::AreNotificationsEmpty() const {
  return notifications->GetRowCount() == 0;
}

NotificationListModel::NotificationListModel(QObject *parent)
    : QVariantListModel(parent),
      last_seen_notification(
          Application::Get().notification.IndexOfLastSeenNotification()) {
  SetRoleNames({{0, "title"},
                {1, "subTitle"},
                {2, "titleColor"},
                {3, "icon"},
                {4, "iconColor"},
                {5, "isSelected"}});
  searchable_roles = {0, 1};
}

QVariantList NotificationListModel::GetRow(int i) const {
  static const Theme kTheme;
  Application &app = Application::Get();
  const Notification &notification = app.notification.GetNotifications()[i];
  QString title_color;
  UiIcon icon;
  if (i <= last_seen_notification) {
    title_color = kTheme.kColorSubText;
  } else {
    title_color = kTheme.kColorText;
  }
  if (notification.is_error) {
    icon.icon = "error";
    icon.color = "red";
  } else {
    icon.icon = "info";
    icon.color = kTheme.kColorSubText;
  }
  bool is_selected = i == GetRowCount() - 1;
  return {notification.title, notification.description,
          title_color,        icon.icon,
          icon.color,         is_selected};
}

int NotificationListModel::GetRowCount() const {
  return Application::Get().notification.GetNotifications().size();
}
