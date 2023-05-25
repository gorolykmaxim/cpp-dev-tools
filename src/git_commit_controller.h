#ifndef GITCOMMITCONTROLLER_H
#define GITCOMMITCONTROLLER_H

#include <QObject>
#include <QQmlEngine>

#include "text_list_model.h"

struct ChangedFile {
  enum Status {
    kNew,
    kModified,
    kDeleted,
  };
  Status status = kNew;
  bool is_staged = false;
  QString path;
};

class ChangedFileListModel : public TextListModel {
 public:
  explicit ChangedFileListModel(QObject* parent);

  QList<ChangedFile> list;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;
};

class GitCommitController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(ChangedFileListModel* files MEMBER files CONSTANT)
  Q_PROPERTY(bool hasChanges READ HasChanges NOTIFY filesChanged)
 public:
  explicit GitCommitController(QObject* parent = nullptr);
  bool HasChanges() const;

 public slots:
  void findChangedFiles();
  void toggleStagedSelectedFile();

 signals:
  void filesChanged();

 private:
  void ExecuteGitCommand(const QStringList& args, const QString& input,
                         const QString& error_title);
  ChangedFileListModel* files;
};

#endif  // GITCOMMITCONTROLLER_H
