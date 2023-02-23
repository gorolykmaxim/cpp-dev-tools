#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>

class QVariantListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(QString filter READ GetFilter WRITE SetFilter NOTIFY filterChanged)
 public:
  explicit QVariantListModel(const QHash<int, QByteArray>& role_names,
                             const QList<int> searchable_roles = {});
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex& index, int role = 0) const override;
  void SetItems(const QList<QVariantList>& items);

 public slots:
  QVariant GetFieldByRoleName(int row, const QString& name) const;
  QVariant GetFieldByRole(int row, int role) const;
  void SetFilter(const QString& filter);
  QString GetFilter();

 signals:
  void filterChanged();

 private:
  void Filter();

  QString filter;
  QList<QVariantList> items;
  QList<QVariantList> items_filtered;
  QList<int> searchable_roles;
  QHash<int, QByteArray> role_names;
  QHash<QString, int> name_to_role;
};
