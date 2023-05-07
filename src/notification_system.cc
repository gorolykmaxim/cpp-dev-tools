#include "notification_system.h"

#include <QDebug>

#define LOG() qDebug() << "[NotificationSystem]"

void NotificationSystem::Post(const Notification &notification) {
  if (notification.is_error) {
    LOG() << "Error:" << notification.title;
  } else {
    LOG() << "Info:" << notification.title;
  }
  notifications.append(notification);
  emit notificationsChanged();
}

void NotificationSystem::MarkAllNotificationsSeenByUser() {
  LOG() << "Marking all notifications as seen by user";
  last_seen_notification = notifications.size() - 1;
  emit notificationsChanged();
}

const QList<Notification> &NotificationSystem::GetNotifications() const {
  return notifications;
}

int NotificationSystem::GetNotSeenNotificationsCount() const {
  if (last_seen_notification < 0) {
    return notifications.size();
  } else {
    return notifications.size() - last_seen_notification - 1;
  }
}

int NotificationSystem::IndexOfLastSeenNotification() const {
  return last_seen_notification;
}

bool NotificationSystem::HasNewErrors() const {
  for (int i = last_seen_notification + 1; i < notifications.size(); i++) {
    if (notifications[i].is_error) {
      return true;
    }
  }
  return false;
}

QString NotificationSystem::GetLastNotSeenNotificationTitle() const {
  if (GetNotSeenNotificationsCount() == 0) {
    return "";
  }
  return notifications.last().title;
}
