#ifndef NOTIFICATIONSYSTEM_H
#define NOTIFICATIONSYSTEM_H

#include <QDateTime>
#include <QObject>

struct Notification {
  explicit Notification(const QString& title = "");

  bool is_error = false;
  QString title;
  QString description;
  QDateTime time;
};

class NotificationSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(int notSeenNotifications READ GetNotSeenNotificationsCount NOTIFY
                 notificationsChanged)
  Q_PROPERTY(bool newErrors READ HasNewErrors NOTIFY notificationsChanged)
  Q_PROPERTY(QString lastNotSeenNotificationTitle READ
                 GetLastNotSeenNotificationTitle NOTIFY notificationsChanged)
 public:
  void Post(const Notification& notification);
  void MarkAllNotificationsSeenByUser();
  const QList<Notification>& GetNotifications() const;
  int GetNotSeenNotificationsCount() const;
  int IndexOfLastSeenNotification() const;
  bool HasNewErrors() const;
  QString GetLastNotSeenNotificationTitle() const;
  void ClearNotifications();

 signals:
  void notificationsChanged();

 private:
  QList<Notification> notifications;
  int last_seen_notification = -1;
};

#endif  // NOTIFICATIONSYSTEM_H
