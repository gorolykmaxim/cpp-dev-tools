#ifndef GITSYSTEM_H
#define GITSYSTEM_H

#include <QObject>

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
  static void Push();
  void Pull();
  bool IsLookingForBranches() const;
  const QList<GitBranch>& GetBranches() const;
  void ClearBranches();
  void CheckoutBranch(int i);
  void DeleteBranch(int i, bool force);
  QString GetCurrentBranchName() const;

 public slots:
  void findBranches();

 signals:
  void isLookingForBranchesChanged();
  void currentBranchChanged();

 private:
  void ExecuteGitCommand(const QStringList& args, const QString& error,
                         const QString& success);

  QList<GitBranch> branches;
  bool is_looking_for_branches = false;
};

#endif  // GITSYSTEM_H
