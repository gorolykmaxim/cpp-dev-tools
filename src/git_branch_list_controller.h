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

  QList<GitBranch> list;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;
};

class FindGitBranches : public QObject {
  Q_OBJECT
 public:
  explicit FindGitBranches(QObject* parent);
  void Run();

  QList<GitBranch> branches;
 signals:
  void finished();

 private:
  void ParseLocalBranches(const QString& output);
  void ParseRemoteBranches(const QString& output);

  int child_processes = 0;
};

class GitBranchListController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      bool showPlaceholder READ ShouldShowPlaceholder NOTIFY isLoadingChanged)
  Q_PROPERTY(bool isLoading MEMBER is_loading NOTIFY isLoadingChanged)
  Q_PROPERTY(GitBranchListModel* branches MEMBER branches CONSTANT)
 public:
  explicit GitBranchListController(QObject* parent = nullptr);
  bool ShouldShowPlaceholder() const;

 signals:
  void isLoadingChanged();

 private:
  void SetIsLoading(bool value);

  bool is_loading = false;
  GitBranchListModel* branches;
};

#endif  // GITBRANCHLISTCONTROLLER_H
