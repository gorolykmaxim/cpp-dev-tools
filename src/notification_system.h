#ifndef NOTIFICATIONSYSTEM_H
#define NOTIFICATIONSYSTEM_H

#include <QObject>

struct Notification {
  bool is_error = false;
  QString title;
  QString description;
  bool seen_by_user = false;
};

class NotificationSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(int notSeenNotifications READ GetNotSeenNotificationsCount NOTIFY
                 notificationsChanged)
 public:
  void Post(Notification notification);
  void MarkAllNotificationsSeenByUser();
  const QList<Notification>& GetNotifications() const;
  int GetNotSeenNotificationsCount() const;

 signals:
  void notificationsChanged();

 private:
  QList<Notification> notifications;
};

#endif  // NOTIFICATIONSYSTEM_H
