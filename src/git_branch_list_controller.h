#ifndef GITBRANCHLISTCONTROLLER_H
#define GITBRANCHLISTCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "git_system.h"
#include "text_list_model.h"

class GitBranchListModel : public TextListModel {
 public:
  explicit GitBranchListModel(QObject* parent);
  const GitBranch* GetSelected() const;

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
  Q_PROPERTY(QString selectedBranchName READ GetSelectedBranchName NOTIFY
                 selectedBranchChanged)
 public:
  explicit GitBranchListController(QObject* parent = nullptr);
  bool IsLocalBranchSelected() const;
  QString GetSelectedBranchName() const;

 public slots:
  void deleteSelected(bool force);
  void checkoutSelected();
  void displayList();

 signals:
  void selectedBranchChanged();

 private:
  GitBranchListModel* branches;
};

#endif  // GITBRANCHLISTCONTROLLER_H
