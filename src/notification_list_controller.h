#ifndef NOTIFICATIONLISTCONTROLLER_H
#define NOTIFICATIONLISTCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "notification_system.h"
#include "qvariant_list_model.h"
#include "ui_icon.h"

class NotificationListModel : public QVariantListModel {
 public:
  static UiIcon GetIconOf(const Notification& notification);
  explicit NotificationListModel(QObject* parent);
  int GetRowCount() const override;
  const Notification& At(int i) const;
  QString GetTitleColorOf(int i) const;

  int last_seen_notification = -1;

 protected:
  QVariantList GetRow(int i) const override;
};

class NotificationListController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(NotificationListModel* notifications MEMBER notifications CONSTANT)
  Q_PROPERTY(bool areNotificationsEmpty READ AreNotificationsEmpty NOTIFY
                 notificationsChanged)
  Q_PROPERTY(int selected MEMBER selected NOTIFY selectedChanged)
  Q_PROPERTY(QString selectedNotificationIcon READ GetSelectedNotificationIcon
                 NOTIFY selectedChanged)
  Q_PROPERTY(QString selectedNotificationIconColor READ
                 GetSelectedNotificationIconColor NOTIFY selectedChanged)
  Q_PROPERTY(QString selectedNotificationTitle READ GetSelectedNotificationTitle
                 NOTIFY selectedChanged)
  Q_PROPERTY(QString selectedNotificationTitleColor READ
                 GetSelectedNotificationTitleColor NOTIFY selectedChanged)
  Q_PROPERTY(QString selectedNotificationDescription READ
                 GetSelectedNotificationDescription NOTIFY selectedChanged)
 public:
  explicit NotificationListController(QObject* parent = nullptr);
  bool AreNotificationsEmpty() const;
  QString GetSelectedNotificationIcon() const;
  QString GetSelectedNotificationIconColor() const;
  QString GetSelectedNotificationTitle() const;
  QString GetSelectedNotificationTitleColor() const;
  QString GetSelectedNotificationDescription() const;

 public slots:
  void displayList();
  void clearNotifications();

 signals:
  void notificationsChanged();
  void selectedChanged();

 private:
  NotificationListModel* notifications;
  int selected = -1;
};

#endif  // NOTIFICATIONLISTCONTROLLER_H
