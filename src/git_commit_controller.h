#ifndef GITCOMMITCONTROLLER_H
#define GITCOMMITCONTROLLER_H

#include <QFontMetrics>
#include <QObject>
#include <QQmlEngine>

#include "os_command.h"
#include "syntax.h"
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
  QString path;
};

class ChangedFileListModel : public TextListModel {
 public:
  explicit ChangedFileListModel(QObject* parent);
  const ChangedFile* GetSelected() const;

  QList<ChangedFile> list;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;
};

enum DiffLineType {
  kHeader = 1,
  kUnchanged = 2,
  kAdded = 4,
  kDeleted = 8,
};

class DiffFormatter : public TextAreaFormatter {
 public:
  explicit DiffFormatter(QObject* parent);
  QList<TextSectionFormat> Format(const QString& text, const QTextBlock& block);

  QList<int> diff_line_flags;
  bool is_side_by_side_diff;
  int line_number_width_before;
  int line_number_width_after;
  QTextCharFormat header_format, added_format, added_placeholder_format,
      deleted_format, deleted_placeholder_format, line_number_format;
  SyntaxFormatter syntax;

 private:
  void FormatSideBySide(const QString& text, const QTextBlock& block, int flags,
                        QList<TextSectionFormat>& fs);
  void FormatUnified(const QString& text, const QTextBlock& block, int flags,
                     QList<TextSectionFormat>& fs);
  void ApplySyntaxFormatter(const QString& text, const QTextBlock& block,
                            TextSection section,
                            const QTextCharFormat* diff_format,
                            QList<TextSectionFormat>& results);
};

class GitCommitController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(ChangedFileListModel* files MEMBER files CONSTANT)
  Q_PROPERTY(bool hasChanges READ HasChanges NOTIFY filesChanged)
  Q_PROPERTY(int sidebarWidth READ CalcSideBarWidth CONSTANT)
  Q_PROPERTY(QString diff MEMBER diff NOTIFY selectedFileChanged)
  Q_PROPERTY(DiffFormatter* formatter MEMBER formatter CONSTANT)
  Q_PROPERTY(bool isSelectedFileModified READ IsSelectedFileModified NOTIFY
                 selectedFileChanged)
  Q_PROPERTY(QString diffError MEMBER diff_error NOTIFY diffErrorChanged)
 public:
  explicit GitCommitController(QObject* parent = nullptr);
  bool HasChanges() const;
  int CalcSideBarWidth() const;
  bool IsSelectedFileModified() const;

 public slots:
  void findChangedFiles();
  void toggleStagedSelectedFile();
  void resetSelectedFile();
  void commit(const QString& msg, bool commit_all, bool amend);
  void loadLastCommitMessage();
  void resizeDiff(int width);
  void toggleUnifiedDiff();

 signals:
  void filesChanged();
  void commitMessageChanged(const QString& msg);
  void selectedFileChanged();
  void diffErrorChanged();

 private:
  Promise<OsProcess> ExecuteGitCommand(const QStringList& args,
                                       const QString& input,
                                       const QString& error_title);
  void DiffSelectedFile();
  void RedrawDiff();
  void DrawSideBySideDiff(const QList<int>& lns_b, const QList<int>& lns_a,
                          int max_chars, int mcln_b, int mcln_a);
  void DrawUnifiedDiff(const QList<int>& lns_b, const QList<int>& lns_a,
                       int max_chars, int mcln);
  void SetDiffError(const QString& text);

  ChangedFileListModel* files;
  QStringList raw_git_diff_output;
  QString diff;
  int diff_width;
  DiffFormatter* formatter;
  bool is_side_by_side_diff;
  QString diff_error;
  QFontMetrics mono_font_metrics;
};

#endif  // GITCOMMITCONTROLLER_H
