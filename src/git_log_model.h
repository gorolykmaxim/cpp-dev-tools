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
  Q_PROPERTY(QString branchOrFile MEMBER branch_or_file NOTIFY optionsChanged)
  Q_PROPERTY(QString searchTerm MEMBER search_term NOTIFY optionsChanged)
  Q_PROPERTY(QString selectedCommitSha READ GetSelectedCommitSha NOTIFY
                 selectedItemChanged)
 public:
  explicit GitLogModel(QObject* parent = nullptr);
  QString GetSelectedCommitSha() const;

  QList<GitCommit> list;

 public slots:
  void setBranchOrFile(const QString& value);
  void setSearchTerm(const QString& value);
  void load(bool only_reselect = false);
  void checkout();
  void cherryPick();

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;

 signals:
  void branchChanged();
  void optionsChanged();

 private:
  void SaveOptions() const;

  QString branch_or_file;
  QString search_term;
};

#endif  // GITLOGMODEL_H
