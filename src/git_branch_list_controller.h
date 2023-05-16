#ifndef GITBRANCHLISTCONTROLLER_H
#define GITBRANCHLISTCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "text_list_model.h"

struct GitBranch {
  QString name;
  bool is_remote = false;
  bool is_current = false;
};

class GitBranchListModel : public TextListModel {
 public:
  explicit GitBranchListModel(QObject* parent);
  const GitBranch* GetSelected() const;

  QList<GitBranch> list;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;
};

class GitBranchListController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(GitBranchListModel* branches MEMBER branches CONSTANT)
  Q_PROPERTY(bool isLocalBranchSelected READ IsLocalBranchSelected NOTIFY
                 selectedBranchChanged)
 public:
  explicit GitBranchListController(QObject* parent = nullptr);
  bool IsLocalBranchSelected() const;

 public slots:
  void deleteSelectedBranch();

 signals:
  void selectedBranchChanged();

 private:
  void FindBranches();

  GitBranchListModel* branches;
};

#endif  // GITBRANCHLISTCONTROLLER_H
