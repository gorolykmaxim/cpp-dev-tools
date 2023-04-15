#ifndef NOTIFICATIONSYSTEM_H
#define NOTIFICATIONSYSTEM_H

#include <QObject>

struct Notification {
  bool is_error = false;
  QString title;
  QString description;
};

class NotificationSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(int notSeenNotifications READ GetNotSeenNotificationsCount NOTIFY
                 notificationsChanged)
 public:
  void Post(const Notification& notification);
  void MarkAllNotificationsSeenByUser();
  const QList<Notification>& GetNotifications() const;
  int GetNotSeenNotificationsCount() const;
  int IndexOfLastSeenNotification() const;

 signals:
  void notificationsChanged();

 private:
  QList<Notification> notifications;
  int last_seen_notification = -1;
};

#endif  // NOTIFICATIONSYSTEM_H
