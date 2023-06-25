#ifndef GITCOMMITCONTROLLER_H
#define GITCOMMITCONTROLLER_H

#include <QFontMetrics>
#include <QObject>
#include <QQmlEngine>

#include "os_command.h"
#include "text_area_controller.h"
#include "text_list_model.h"

struct ChangedFile {
  enum Status {
    kNew,
    kModified,
    kDeleted,
  };
  Status status = kNew;
  bool is_staged = false;
  int additions = 0;
  int removals = 0;
  QString path;
};

class ChangedFileListModel : public TextListModel {
  Q_OBJECT
  Q_PROPERTY(QString selectedFileName READ GetSelectedFileName NOTIFY
                 selectedItemChanged)
  Q_PROPERTY(QString stats READ GetStats NOTIFY statsChanged)
 public:
  explicit ChangedFileListModel(QObject* parent);
  const ChangedFile* GetSelected() const;
  QString GetSelectedFileName() const;
  QString GetStats() const;

  QList<ChangedFile> list;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;

 signals:
  void statsChanged();

 private:
  static QString FormatStats(int additions, int removals);
};

class CommitMessageFormatter : public TextFormatter {
 public:
  explicit CommitMessageFormatter(QObject* parent);
  QList<TextFormat> Format(const QString& text, LineInfo line) const;

 private:
  void FormatSuffixAfter(int offset, int length, QList<TextFormat>& fs) const;

  QTextCharFormat format;
};

class GitCommitController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(ChangedFileListModel* files MEMBER files CONSTANT)
  Q_PROPERTY(bool hasChanges READ HasChanges NOTIFY filesChanged)
  Q_PROPERTY(int sidebarWidth READ CalcSideBarWidth CONSTANT)
  Q_PROPERTY(QString diffError MEMBER diff_error NOTIFY diffErrorChanged)
  Q_PROPERTY(QString rawDiff MEMBER raw_diff NOTIFY rawDiffChanged)
  Q_PROPERTY(CommitMessageFormatter* formatter MEMBER formatter CONSTANT)
 public:
  explicit GitCommitController(QObject* parent = nullptr);
  bool HasChanges() const;
  int CalcSideBarWidth() const;

 public slots:
  void findChangedFiles();
  void toggleStagedSelectedFile();
  void resetSelectedFile();
  void commit(const QString& msg, bool commit_all, bool amend);
  void loadLastCommitMessage();
  void rollbackChunk(int chunk, int chunk_count);

 signals:
  void filesChanged();
  void commitMessageChanged(const QString& msg);
  void diffErrorChanged();
  void rawDiffChanged();

 private:
  Promise<OsProcess> ExecuteGitCommand(const QStringList& args,
                                       const QString& input,
                                       const QString& error_title);
  void DiffSelectedFile();
  void SetDiff(const QString& diff, const QString& error);

  ChangedFileListModel* files;
  QString diff_error;
  QString raw_diff;
  CommitMessageFormatter* formatter;
};

#endif  // GITCOMMITCONTROLLER_H
