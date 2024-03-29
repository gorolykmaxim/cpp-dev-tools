#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QQueue>
#include <functional>

#include "placeholder.h"

class UiCommandBuffer : public QObject {
  Q_OBJECT
 public:
  UiCommandBuffer();
  void ScheduleCommands(int items, int items_per_cmd,
                        const std::function<void(int, int)>& cmd);
  void ScheduleCommand(const std::function<void()>& cmd);
  void RunCommands();
  void Clear();
 signals:
  void commandsReady();

 private:
  void ExecuteCommand();

  QQueue<std::function<void()>> commands;
};

struct TextListItem {
  int index = 0;
  QVariantList fields;
};

class TextListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(QString filter READ GetFilter WRITE SetFilterIfChanged NOTIFY
                 filterChanged)
  Q_PROPERTY(bool isUpdating MEMBER is_updating NOTIFY isUpdatingChanged)
  Q_PROPERTY(
      QString placeholderText READ GetPlaceholderText NOTIFY placeholderChanged)
  Q_PROPERTY(QString placeholderColor READ GetPlaceholderColor NOTIFY
                 placeholderChanged)
 public:
  explicit TextListModel(QObject* parent);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex& index, int role = 0) const override;
  void Load(int item_to_select = 0);
  void LoadNew(int starting_from, int item_to_select = 0);
  void LoadRemoved(int count);
  int GetSelectedItemIndex() const;
  QString GetPlaceholderText() const;
  QString GetPlaceholderColor() const;
  void SetEmptyListPlaceholder(const QString& text = "",
                               const QString& color = "");
  void SetPlaceholder(const QString& text = "", const QString& color = "");
  bool IsUpdating() const;
  void ReSelectItem(int index);

  int min_filter_sub_match_length = 2;

 public slots:
  QVariant getFieldByRoleName(int row, const QString& name) const;
  void selectItemByIndex(int i);
  bool SetFilterIfChanged(const QString& filter);
  bool SetFilter(const QString& filter);
  QString GetFilter();

 signals:
  void filterChanged();
  void preSelectCurrentIndex(int index);
  void isUpdatingChanged();
  void selectedItemChanged();
  void placeholderChanged();

 protected:
  virtual QVariantList GetRow(int i) const = 0;
  virtual int GetRowCount() const = 0;
  void SetRoleNames(const QHash<int, QByteArray>& role_names);
  void SetIsUpdating(bool is_updating);

  QString filter;
  QList<TextListItem> items;
  QList<int> searchable_roles;
  QHash<int, QByteArray> role_names;
  QHash<QString, int> name_to_role;
  UiCommandBuffer cmd_buffer;
  bool is_updating = false;
  int selected_item_index = -1;

 private:
  Placeholder empty_list_placeholder;
  Placeholder placeholder;
};
