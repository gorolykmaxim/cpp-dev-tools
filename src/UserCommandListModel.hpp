#pragma once

#include <functional>

#include "QVariantListModel.hpp"

class UserCommand {
 public:
  QString GetFormattedShortcut() const;

  QString group;
  QString name;
  QString shortcut;
  std::function<void()> callback;
};

class UserCommandListModel : public QVariantListModel {
 public:
  explicit UserCommandListModel(QObject* parent);
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;

  QList<UserCommand> list;
};
