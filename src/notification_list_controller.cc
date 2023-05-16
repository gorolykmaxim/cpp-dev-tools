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
  QObject::connect(
      &app.notification, &NotificationSystem::notificationsChanged, this,
      [this, &app] {
        notifications->Load(app.notification.GetNotifications().size() - 1);
      });
  QObject::connect(notifications, &TextListModel::selectedItemChanged, this,
                   [this] { emit selectedChanged(); });
}

QString NotificationListController::GetSelectedNotificationIcon() const {
  if (auto s = notifications->GetSelected()) {
    return NotificationListModel::GetIconOf(*s).icon;
  }
  return "";
}

QString NotificationListController::GetSelectedNotificationIconColor() const {
  if (auto s = notifications->GetSelected()) {
    return NotificationListModel::GetIconOf(*s).color;
  }
  return "";
}

QString NotificationListController::GetSelectedNotificationTitle() const {
  if (auto s = notifications->GetSelected()) {
    return s->title;
  } else {
    return "";
  }
}

QString NotificationListController::GetSelectedNotificationTitleColor() const {
  int i = notifications->GetSelectedItemIndex();
  return i < 0 ? "" : notifications->GetTitleColorOf(i);
}

QString NotificationListController::GetSelectedNotificationTime() const {
  if (auto s = notifications->GetSelected()) {
    return s->time.toString(Application::kDateTimeFormat);
  }
  return "";
}

QString NotificationListController::GetSelectedNotificationDescription() const {
  if (auto s = notifications->GetSelected()) {
    return s->description;
  } else {
    return "";
  }
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
    : TextListModel(parent) {
  SetRoleNames({{0, "title"},
                {1, "titleColor"},
                {2, "subTitle"},
                {3, "icon"},
                {4, "iconColor"}});
  searchable_roles = {0, 2};
  SetEmptyListPlaceholder(Placeholder("No notifications found"));
}

QVariantList NotificationListModel::GetRow(int i) const {
  const Notification &notification = At(i);
  UiIcon icon = GetIconOf(notification);
  QString title_color = GetTitleColorOf(i);
  return {notification.title, title_color,
          notification.time.toString(Application::kDateTimeFormat), icon.icon,
          icon.color};
}

int NotificationListModel::GetRowCount() const {
  return Application::Get().notification.GetNotifications().size();
}

QString NotificationListModel::GetTitleColorOf(int i) const {
  static const Theme kTheme;
  if (i <= last_seen_notification) {
    return kTheme.kColorSubText;
  } else {
    return kTheme.kColorText;
  }
}

const Notification *NotificationListModel::GetSelected() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? nullptr : &At(i);
}

const Notification &NotificationListModel::At(int i) const {
  return Application::Get().notification.GetNotifications()[i];
}
