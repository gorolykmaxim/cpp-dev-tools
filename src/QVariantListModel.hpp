#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <functional>

class QVariantListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(QString filter READ GetFilter WRITE SetFilter NOTIFY filterChanged)

  using GetRow = std::function<QVariantList(int)>;
  using GetRowCount = std::function<int()>;

 public:
  explicit QVariantListModel(const GetRow& get_row,
                             const GetRowCount& get_row_count,
                             const QHash<int, QByteArray>& role_names,
                             const QList<int> searchable_roles);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex& index, int role = 0) const override;
  void LoadItems();

 public slots:
  QVariant GetFieldByRoleName(int row, const QString& name) const;
  QVariant GetFieldByRole(int row, int role) const;
  void SetFilter(const QString& filter);
  QString GetFilter();

 signals:
  void filterChanged();

 private:
  QString filter;
  GetRow get_row;
  GetRowCount get_row_count;
  QList<QVariantList> items;
  QList<int> searchable_roles;
  QHash<int, QByteArray> role_names;
  QHash<QString, int> name_to_role;
};
