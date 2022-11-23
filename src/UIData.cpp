#include "UIData.hpp"
#include "AppData.hpp"
#include "Process.hpp"

QVariantListModel::QVariantListModel(const QHash<int, QByteArray>& role_names)
    : QAbstractListModel(), role_names(role_names) {}

int QVariantListModel::rowCount(const QModelIndex&) const {
  return items.size();
}

QHash<int, QByteArray> QVariantListModel::roleNames() const {
  return role_names;
}

QVariant QVariantListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }
  if (index.row() < 0 || index.row() >= items.size()) {
    return QVariant();
  }
  const QVariantList& row = items[index.row()];
  Q_ASSERT(role >= 0 && role < row.size());
  return row[role];
}

void QVariantListModel::SetItems(const QList<QVariantList>& items) {
  beginResetModel();
  this->items = items;
  endResetModel();
}

UIActionRouter::UIActionRouter(AppData& app) : app(app) {}

void UIActionRouter::OnAction(const QString& type, const QVariantList& args) {
  // In some cases this might get called WHILE we are in the middle
  // of the setContextProperties(), which can break some clients that
  // expect user action callbacks to always be called asynchronously.
  // This is why we are invoking it later instead of immediately right here.
  QMetaObject::invokeMethod(&app.gui_engine, [this, type, args] () {
    app.events.enqueue(Event(type, args));
    ExecuteProcesses(app);
  }, Qt::QueuedConnection);
}
