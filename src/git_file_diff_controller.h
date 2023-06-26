#ifndef GITFILEDIFFCONTROLLER_H
#define GITFILEDIFFCONTROLLER_H

#include <QObject>
#include <QQmlEngine>

#include "text_list_model.h"

class GitFileDiffModel : public TextListModel {
  Q_OBJECT
  Q_PROPERTY(bool moreThanOne READ HasMoreThanOne NOTIFY filesChanged)
  Q_PROPERTY(QString selected READ GetSelectedFile NOTIFY selectedItemChanged)
 public:
  explicit GitFileDiffModel(QObject* parent);
  bool HasMoreThanOne() const;
  QString GetSelectedFile() const;

  QStringList files;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;

 signals:
  void filesChanged();
};

class GitFileDiffController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString filePath MEMBER file_path NOTIFY optionsChanged)
  Q_PROPERTY(QString branchesToCompare MEMBER branches_to_compare NOTIFY
                 optionsChanged)
  Q_PROPERTY(QString rawDiff MEMBER raw_diff NOTIFY rawDiffChanged)
  Q_PROPERTY(QString diffError MEMBER diff_error NOTIFY rawDiffChanged)
  Q_PROPERTY(GitFileDiffModel* files MEMBER files CONSTANT)
 public:
  explicit GitFileDiffController(QObject* parent = nullptr);
  QString GetFile() const;

 public slots:
  void setBranchesToCompare(const QString& value);
  void setFilePath(const QString& value);
  void showDiff();

 signals:
  void optionsChanged();
  void rawDiffChanged();

 private:
  void SetDiff(const QString& diff, const QString& error);
  void SaveOptions() const;
  void DiffSelectedFile();

  QString branches_to_compare;
  QString file_path;
  QString raw_diff;
  QString diff_error;
  GitFileDiffModel* files;
};

#endif  // GITFILEDIFFCONTROLLER_H
