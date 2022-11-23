#pragma once

#include "Lib.hpp"

const QString kViewSlot = "viewSlot";
const QString kDialogSlot = "dialogSlot";
const QString kStatusSlot = "statusSlot";
const QString kDialogVisible = "dVisible";
const QString kWindowTitle = "windowTitle";

using UserActionHandler = std::function<void(const QVariantList&)>;
using DialogActionHandler = std::function<void()>;
using UIDataField = QQmlContext::PropertyPair;

struct UIListField {
  QString name;
  QHash<int, QByteArray> role_names;
  QList<QVariantList> items;
};

struct QVariantListModel: public QAbstractListModel {
public:
  explicit QVariantListModel(const QHash<int, QByteArray>& role_names);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex& index, int role = 0) const override;
  void SetItems(const QList<QVariantList>& items);
private:
  QList<QVariantList> items;
  QHash<int, QByteArray> role_names;
};

struct ViewData {
  QSet<QString> data_field_names;
  QHash<QString, QPtr<QVariantListModel>> list_fields;
  QList<QString> event_types;
};

struct AppData;

struct UIActionRouter: public QObject {
  Q_OBJECT
public:
  UIActionRouter(AppData& app);
public slots:
  void OnAction(const QString& type, const QVariantList& args);
private:
  AppData& app;
};
