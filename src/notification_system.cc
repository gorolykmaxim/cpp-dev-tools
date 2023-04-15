#include "notification_system.h"

#include <QDebug>

#define LOG() qDebug() << "[NotificationSystem]"

void NotificationSystem::Post(Notification notification) {
  if (notification.is_error) {
    LOG() << "Error:" << notification.title;
  } else {
    LOG() << "Info:" << notification.title;
  }
  notification.seen_by_user = false;
  notifications.append(std::move(notification));
  emit notificationsChanged();
}

void NotificationSystem::MarkAllNotificationsSeenByUser() {
  LOG() << "Marking all notifications as seen by user";
  for (Notification notification : notifications) {
    notification.seen_by_user = true;
  }
  emit notificationsChanged();
}

const QList<Notification> &NotificationSystem::GetNotifications() const {
  return notifications;
}

int NotificationSystem::GetNotSeenNotificationsCount() const {
  int result = 0;
  for (const Notification &notification : notifications) {
    if (!notification.seen_by_user) {
      result++;
    }
  }
  return result;
}
