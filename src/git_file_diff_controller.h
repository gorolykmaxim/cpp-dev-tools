#ifndef GITFILEDIFFCONTROLLER_H
#define GITFILEDIFFCONTROLLER_H

#include <QObject>
#include <QQmlEngine>

class GitFileDiffController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString file READ GetFile NOTIFY fileChanged)
  Q_PROPERTY(QString rawDiff MEMBER raw_diff NOTIFY rawDiffChanged)
  Q_PROPERTY(QString diffError MEMBER diff_error NOTIFY rawDiffChanged)
 public:
  explicit GitFileDiffController(QObject* parent = nullptr);
  QString GetFile() const;

 public slots:
  void setBranchesToCompare(const QString& value);
  void setFilePath(const QString& value);
  void showDiff();

 signals:
  void fileChanged();
  void rawDiffChanged();

 private:
  void SetDiff(const QString& diff, const QString& error);

  QString branches_to_compare;
  QString file_path;
  QString raw_diff;
  QString diff_error;
};

#endif  // GITFILEDIFFCONTROLLER_H
