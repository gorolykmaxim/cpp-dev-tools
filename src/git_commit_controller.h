#ifndef GITCOMMITCONTROLLER_H
#define GITCOMMITCONTROLLER_H

#include <QObject>
#include <QQmlEngine>

#include "text_list_model.h"

class ChangedFileListModel : public TextListModel {
 public:
  explicit ChangedFileListModel(QObject* parent);

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
