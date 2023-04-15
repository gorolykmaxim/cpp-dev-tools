#ifndef NOTIFICATIONLISTCONTROLLER_H
#define NOTIFICATIONLISTCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "qvariant_list_model.h"

class NotificationListModel : public QVariantListModel {
 public:
  explicit NotificationListModel(QObject* parent);
  int GetRowCount() const override;

 protected:
  QVariantList GetRow(int i) const override;

 private:
  int last_seen_notification;
};

class NotificationListController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(NotificationListModel* notifications MEMBER notifications CONSTANT)
  Q_PROPERTY(bool areNotificationsEmpty READ AreNotificationsEmpty NOTIFY
                 notificationsChanged)
 public:
  explicit NotificationListController(QObject* parent = nullptr);
  bool AreNotificationsEmpty() const;

 signals:
  void notificationsChanged();

 private:
  NotificationListModel* notifications;
};

#endif  // NOTIFICATIONLISTCONTROLLER_H
