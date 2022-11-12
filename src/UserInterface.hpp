#pragma once

#include "Lib.hpp"

const QString kViewSlot = "viewSlot";
const QString kDialogSlot = "dialogSlot";
const QString kDataDialogVisible = "dataDialogVisible";
const QString kDataWindowTitle = "dataWindowTitle";

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

struct ViewData {
  QSet<QString> data_field_names;
  QHash<QString, QPtr<QVariantListModel>> list_fields;
  QHash<QString, UserActionHandler> user_action_handlers;
};

class UserInterface: public QObject {
  Q_OBJECT
public:
  UserInterface();
  void DisplayView(
      const QString& slot_name, const QString& qml_file,
      const QList<DataField>& data_fields, const QList<ListField>& list_fields,
      const QHash<QString, UserActionHandler>& user_action_handlers);
  void SetDataField(const QString& slot_name, const QString& name,
                    const QVariant& value);
  QVariantListModel& GetListField(const QString& slot_name,
                                  const QString& name);
  void DisplayInputAndListView(
    const QString& title, const QString& input_value,
    const QString& button_text, const QVector<QVariantList>& list_items,
    const std::function<void(const QString&)>& on_input_value_changed,
    const std::function<void()>& on_enter_pressed,
    const std::function<void(int)>& on_item_selected);
  void SetInputAndListViewItems(const QVector<QVariantList>& list_items);
  void SetInputAndListViewInput(const QString& value);
  void SetInputAndListViewButtonText(const QString& value);
  void DisplayAlertDialog(const QString& title, const QString& text,
                          bool error = true, bool cancellable = false,
                          const std::function<void()>& on_ok = nullptr,
                          const std::function<void()>& on_cancel = nullptr);
  void DisplayTextView(const QString& title, const QString& text);
public slots:
  void OnUserAction(const QString& slot_name, const QString& action,
                    const QVariantList& args);
private:
  QHash<QString, ViewData> view_slot_name_to_data;
  QQmlApplicationEngine engine;
};
