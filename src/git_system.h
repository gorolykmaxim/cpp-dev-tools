#ifndef GITSYSTEM_H
#define GITSYSTEM_H

#include <QObject>

#include "os_command.h"
#include "promise.h"

struct GitBranch {
  QString name;
  bool is_remote = false;
  bool is_current = false;
};

class GitSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString currentBranch READ GetCurrentBranchName NOTIFY
                 currentBranchChanged)
 public:
  static QList<QString> FindIgnoredPathsSync();
  void Push();
  void Pull();
  bool IsLookingForBranches() const;
  const QList<GitBranch>& GetBranches() const;
  void ClearBranches();
  void CheckoutBranch(int i);
  void DeleteBranch(int i, bool force);
  QString GetCurrentBranchName() const;
  Promise<OsProcess> CreateBranch(const QString& name, const QString& basis);
  void FindBranches();

 public slots:
  void refreshBranchesIfProjectSelected();

 signals:
  void isLookingForBranchesChanged();
  void currentBranchChanged();

 private:
  Promise<OsProcess> ExecuteGitCommand(const QStringList& args,
                                       const QString& error,
                                       const QString& success);

  QList<GitBranch> branches;
  bool is_looking_for_branches = false;
};

#endif  // GITSYSTEM_H
