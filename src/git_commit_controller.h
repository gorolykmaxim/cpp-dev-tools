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
 public:
  explicit GitCommitController(QObject* parent = nullptr);

 private:
  ChangedFileListModel* files;
};

#endif  // GITCOMMITCONTROLLER_H
