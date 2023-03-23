#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QQueue>
#include <functional>

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

class QVariantListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(QString filter READ GetFilter WRITE SetFilterIfChanged NOTIFY
                 filterChanged)
 public:
  explicit QVariantListModel(QObject* parent);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex& index, int role = 0) const override;
  void Load();

 public slots:
  QVariant GetFieldByRoleName(int row, const QString& name) const;
  QVariant GetFieldByRole(int row, int role) const;
  void SetFilterIfChanged(const QString& filter);
  void SetFilter(const QString& filter);
  QString GetFilter();

 signals:
  void filterChanged();
  void loadingComplete();

 protected:
  virtual QVariantList GetRow(int i) const;
  virtual int GetRowCount() const;
  void SetRoleNames(const QHash<int, QByteArray>& role_names);

  QString filter;
  QList<QVariantList> items;
  QList<int> searchable_roles;
  QHash<int, QByteArray> role_names;
  QHash<QString, int> name_to_role;
  UiCommandBuffer cmd_buffer;
};

class SimpleQVariantListModel : public QVariantListModel {
 public:
  SimpleQVariantListModel(QObject* parent,
                          const QHash<int, QByteArray>& role_names,
                          const QList<int>& searchable_roles);
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;

  QList<QVariantList> list;
};
