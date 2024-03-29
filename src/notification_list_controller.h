#ifndef NOTIFICATIONLISTCONTROLLER_H
#define NOTIFICATIONLISTCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "notification_system.h"
#include "text_list_model.h"
#include "ui_icon.h"

class NotificationListModel : public TextListModel {
 public:
  static UiIcon GetIconOf(const Notification& notification);
  explicit NotificationListModel(QObject* parent);
  int GetRowCount() const override;
  QString GetTitleColorOf(int i) const;
  const Notification* GetSelected() const;
  const Notification& At(int i) const;

  int last_seen_notification = -1;

 protected:
  QVariantList GetRow(int i) const override;
};

class NotificationListController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(NotificationListModel* notifications MEMBER notifications CONSTANT)
  Q_PROPERTY(QString selectedNotificationIcon READ GetSelectedNotificationIcon
                 NOTIFY selectedChanged)
  Q_PROPERTY(QString selectedNotificationIconColor READ
                 GetSelectedNotificationIconColor NOTIFY selectedChanged)
  Q_PROPERTY(QString selectedNotificationTitle READ GetSelectedNotificationTitle
                 NOTIFY selectedChanged)
  Q_PROPERTY(QString selectedNotificationTitleColor READ
                 GetSelectedNotificationTitleColor NOTIFY selectedChanged)
  Q_PROPERTY(QString selectedNotificationTime READ GetSelectedNotificationTime
                 NOTIFY selectedChanged)
  Q_PROPERTY(QString selectedNotificationDescription READ
                 GetSelectedNotificationDescription NOTIFY selectedChanged)
 public:
  explicit NotificationListController(QObject* parent = nullptr);
  QString GetSelectedNotificationIcon() const;
  QString GetSelectedNotificationIconColor() const;
  QString GetSelectedNotificationTitle() const;
  QString GetSelectedNotificationTitleColor() const;
  QString GetSelectedNotificationTime() const;
  QString GetSelectedNotificationDescription() const;

 public slots:
  void displayList();
  void clearNotifications();

 signals:
  void selectedChanged();

 private:
  NotificationListModel* notifications;
};

#endif  // NOTIFICATIONLISTCONTROLLER_H
