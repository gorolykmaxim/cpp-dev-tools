#ifndef GITLOGMODEL_H
#define GITLOGMODEL_H

#include <QObject>
#include <QQmlEngine>

#include "text_list_model.h"

struct GitCommit {
  QString sha;
  QString title;
  QString author;
  QString date;
};

class GitLogModel : public TextListModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString searchTerm MEMBER search_term NOTIFY searchTermChanged)
  Q_PROPERTY(QString selectedCommitSha READ GetSelectedCommitSha NOTIFY
                 selectedItemChanged)
 public:
  explicit GitLogModel(QObject* parent = nullptr);
  QString GetSelectedCommitSha() const;

  QList<GitCommit> list;

 public slots:
  void setBranchOrFile(const QString& value);
  void load();
  void checkout();
  void cherryPick();

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;

 signals:
  void branchChanged();
  void searchTermChanged();

 private:
  QString branch_or_file;
  QString search_term;
};

#endif  // GITLOGMODEL_H
