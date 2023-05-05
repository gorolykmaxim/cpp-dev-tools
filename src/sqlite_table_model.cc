#include "sqlite_table_model.h"

#include <QClipboard>
#include <QGuiApplication>

#include "application.h"

SqliteTableModel::SqliteTableModel(QObject *parent)
    : QAbstractTableModel(parent) {
  current = index(0, 0);
}

int SqliteTableModel::rowCount(const QModelIndex &) const {
  return rows.size();
}

int SqliteTableModel::columnCount(const QModelIndex &) const {
  return columns.size();
}

QVariant SqliteTableModel::data(const QModelIndex &index, int role) const {
  if (role == 0) {
    QVariant value = rows[index.row()][index.column()];
    return value.isNull() ? "NULL" : value;
  } else {
    return index.row() == current.row() && index.column() == current.column();
  }
}

QVariant SqliteTableModel::headerData(int section, Qt::Orientation, int) const {
  return columns[section];
}

QHash<int, QByteArray> SqliteTableModel::roleNames() const {
  return {{0, "display"}, {1, "current"}};
}

void SqliteTableModel::SetTable(const QList<QString> &new_columns,
                                const QList<QVariantList> &new_rows) {
  QModelIndex i;
  beginRemoveColumns(i, 0, columns.size() - 1);
  columns.clear();
  endRemoveColumns();
  beginRemoveRows(i, 0, rows.size() - 1);
  rows.clear();
  endRemoveRows();
  beginInsertColumns(i, 0, new_columns.size() - 1);
  columns = new_columns;
  endInsertColumns();
  beginInsertRows(i, 0, new_rows.size() - 1);
  rows = new_rows;
  endInsertRows();
  current = index(0, 0);
}

bool SqliteTableModel::moveCurrent(int key, int rows_visible) {
  QModelIndex next;
  switch (key) {
    case Qt::Key_Up:
      next = index(current.row() - 1, current.column());
      break;
    case Qt::Key_Down:
      next = index(current.row() + 1, current.column());
      break;
    case Qt::Key_Left:
      next = index(current.row(), current.column() - 1);
      break;
    case Qt::Key_Right:
      next = index(current.row(), current.column() + 1);
      break;
    case Qt::Key_PageUp:
      next = index(current.row() - rows_visible, current.column());
      break;
    case Qt::Key_PageDown:
      next = index(current.row() + rows_visible, current.column());
      break;
    default:
      break;
  }
  return SetCurrent(next);
}

void SqliteTableModel::setCurrent(int row, int column) {
  SetCurrent(index(row, column));
}

void SqliteTableModel::copyCurrentValue() {
  QVariant value = data(current, 0);
  QGuiApplication::clipboard()->setText(value.toString());
}

void SqliteTableModel::displayCurrentValueInDialog() {
  AlertDialog dialog("Cell Value", data(current, 0).toString());
  dialog.flags = AlertDialog::kNoFlags;
  Application::Get().view.DisplayAlertDialog(dialog);
}

bool SqliteTableModel::SetCurrent(QModelIndex next) {
  if (next.isValid()) {
    QModelIndex prev = current;
    current = next;
    emit dataChanged(prev, prev);
    emit dataChanged(current, current);
    emit currentChanged(current);
    return true;
  } else {
    return false;
  }
}
