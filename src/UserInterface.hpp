#pragma once

#include <functional>
#include <QString>
#include <QVariant>
#include <QQmlContext>
#include <QHash>
#include <QByteArray>
#include <QVector>
#include <QObject>
#include <QList>
#include <QSet>
#include <QQmlApplicationEngine>
#include <QAbstractListModel>
#include <QModelIndex>
#include "Common.hpp"

const QString kQmlCurrentView = "currentView";

using UserActionHandler = std::function<void(const QVariantList&)>;
using DialogActionHandler = std::function<void()>;
using DataField = QQmlContext::PropertyPair;

struct ListField {
  QString name;
  QHash<int, QByteArray> role_names;
  QVector<QVariantList> items;
};

class QVariantListModel: public QAbstractListModel {
public:
  explicit QVariantListModel(const QHash<int, QByteArray>& role_names);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex& index, int role = 0) const override;
  void SetItems(const QVector<QVariantList>& items);
private:
  QVector<QVariantList> items;
  QHash<int, QByteArray> role_names;
};

class UserInterface: public QObject {
  Q_OBJECT
public:
  UserInterface();
  void DisplayView(
      const QString& qml_file,
      const QList<DataField>& data_fields,
      const QList<ListField>& list_fields,
      const QHash<QString, UserActionHandler> user_action_handlers);
  void DisplayDialog(const QString& title, const QString& text,
                     bool error = true, bool cancellable = false,
                     const DialogActionHandler& accept_handler = nullptr,
                     const DialogActionHandler& reject_handler = nullptr);
  void SetDataField(const QString& name, const QVariant& value);
  QVariantListModel& GetListField(const QString& name);
public slots:
  void OnUserAction(const QString& action, const QVariantList& args);
  void OnDialogResult(bool result);
private:
  QSet<QString> data_field_names;
  QHash<QString, UserActionHandler> user_action_handlers;
  DialogActionHandler dialog_reject_handler, dialog_accept_handler;
  QHash<QString, QPtr<QVariantListModel>> list_fields;
  QQmlApplicationEngine engine;
};
