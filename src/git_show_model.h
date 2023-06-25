#ifndef GITSHOWMODEL_H
#define GITSHOWMODEL_H

#include <QQmlEngine>

#include "text_list_model.h"

struct GitShowFile {
  QString path;
  int additions = 0;
  int removals = 0;
};

class GitShowModel : public TextListModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString sha MEMBER sha NOTIFY shaChanged)
  Q_PROPERTY(int sidebarWidth READ CalSidebarWidth CONSTANT)
  Q_PROPERTY(QString stats READ GetStats NOTIFY changeListChanged)
  Q_PROPERTY(bool hasChanges READ HasChanges NOTIFY changeListChanged)
  Q_PROPERTY(
      QString rawCommitInfo MEMBER raw_commit_info NOTIFY changeListChanged)
  Q_PROPERTY(QString selectedFileName READ GetSelectedFileName NOTIFY
                 selectedItemChanged)
  Q_PROPERTY(QString rawDiff MEMBER raw_diff NOTIFY rawDiffChanged)
  Q_PROPERTY(QString diffError MEMBER diff_error NOTIFY rawDiffChanged)
 public:
  explicit GitShowModel(QObject* parent = nullptr);
  int CalSidebarWidth() const;
  QString GetStats() const;
  bool HasChanges() const;
  QString GetSelectedFileName() const;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;

 signals:
  void shaChanged();
  void rawDiffChanged();
  void changeListChanged();

 private:
  void FetchCommitInfo();
  void DiffSelectedFile();
  void SetDiff(const QString& diff, const QString& error);

  QString sha;
  QList<GitShowFile> files;
  QString raw_commit_info;
  QString raw_diff;
  QString diff_error;
};

#endif  // GITSHOWMODEL_H
