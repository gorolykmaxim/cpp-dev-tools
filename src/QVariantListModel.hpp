#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QQueue>
#include <functional>

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
  void ExecuteUiUpdateCommands();

 signals:
  void filterChanged();
  void uiUpdateCommandsReady();

 protected:
  virtual QVariantList GetRow(int i) const;
  virtual int GetRowCount() const;
  void SetRoleNames(const QHash<int, QByteArray>& role_names);

  QString filter;
  QList<QVariantList> items;
  QList<int> searchable_roles;
  QHash<int, QByteArray> role_names;
  QHash<QString, int> name_to_role;
  QQueue<std::function<void()>> ui_update_commands;
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
