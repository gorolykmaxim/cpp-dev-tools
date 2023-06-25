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
  Q_PROPERTY(QString branch MEMBER branch NOTIFY branchChanged)
  Q_PROPERTY(QString searchTerm MEMBER search_term NOTIFY searchTermChanged)
 public:
  explicit GitLogModel(QObject* parent = nullptr);

  QList<GitCommit> list;

 public slots:
  void load();
  void checkout();

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;

 signals:
  void branchChanged();
  void searchTermChanged();

 private:
  QString branch;
  QString search_term;
};

#endif  // GITLOGMODEL_H
