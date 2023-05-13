#ifndef FINDINFILESCONTROLLER_H
#define FINDINFILESCONTROLLER_H

#include <QAbstractListModel>
#include <QObject>
#include <QtQmlIntegration>
#include <atomic>

#include "io_task.h"
#include "syntax.h"

struct FileSearchResult {
  QString match;
  QString file_path;
  int column = -1;
  int row = -1;
  int offset = -1;
  int match_length = -1;
};

class FileSearchResultListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(bool isUpdating MEMBER is_updating CONSTANT)
 public:
  explicit FileSearchResultListModel(QObject* parent);
  int rowCount(const QModelIndex& parent) const override;
  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex& index, int role) const override;
  void Clear();
  void Append(const QList<FileSearchResult>& items);
  int CountUniqueFiles() const;
  const FileSearchResult& At(int i) const;

 signals:
  void preSelectCurrentIndex(int);

 private:
  bool is_updating = false;
  QSet<QString> unique_file_paths;
  QList<FileSearchResult> list;
};

struct FindInFilesOptions {
  Q_GADGET
  Q_PROPERTY(bool matchCase MEMBER match_case)
  Q_PROPERTY(bool matchWholeWord MEMBER match_whole_word)
  Q_PROPERTY(bool regexp MEMBER regexp)
  Q_PROPERTY(
      bool includeExternalSearchFolders MEMBER include_external_search_folders)
  Q_PROPERTY(QString filesToInclude MEMBER files_to_include)
  Q_PROPERTY(QString filesToExclude MEMBER files_to_exclude)
  Q_PROPERTY(bool excludeGitIgnoredFiles MEMBER exclude_git_ignored_files)
 public:
  bool match_case = false;
  bool match_whole_word = false;
  bool regexp = false;
  bool include_external_search_folders = false;
  QString files_to_include;
  QString files_to_exclude;
  bool exclude_git_ignored_files = true;

  bool operator==(const FindInFilesOptions& another) const;
  bool operator!=(const FindInFilesOptions& another) const;
};

class FindInFilesTask : public BaseIoTask {
  Q_OBJECT
 public:
  FindInFilesTask(const QString& search_term, FindInFilesOptions options);

  std::atomic<bool> is_cancelled = false;
 signals:
  void resultsFound(QList<FileSearchResult> result);

 protected:
  void RunInBackground() override;

 private:
  QList<FileSearchResult> Find(const QString& line, int column, int offset,
                               const QString& file_name) const;
  QList<FileSearchResult> FindRegex(const QString& line, int column, int offset,
                                    const QString& file_name) const;

  QString search_term;
  QRegularExpression search_term_regex;
  QString folder;
  FindInFilesOptions options;
};

class FileSearchResultFormatter : public TextAreaFormatter {
 public:
  explicit FileSearchResultFormatter(QObject* parent);
  QList<TextSectionFormat> Format(const QString& text, const QTextBlock& block);
  void SetResult(const FileSearchResult& result);

 private:
  SyntaxFormatter* syntax_formatter;
  FileSearchResult result;
  QTextCharFormat result_format;
};

class FindInFilesController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString searchTerm MEMBER search_term NOTIFY searchTermChanged)
  Q_PROPERTY(FileSearchResultListModel* results MEMBER search_results CONSTANT)
  Q_PROPERTY(
      QString searchStatus READ GetSearchStatus NOTIFY searchStatusChanged)
  Q_PROPERTY(FindInFilesOptions options MEMBER options NOTIFY optionsChanged)
  Q_PROPERTY(QString selectedFilePath MEMBER selected_file_path NOTIFY
                 selectedResultChanged)
  Q_PROPERTY(QString selectedFileContent MEMBER selected_file_content NOTIFY
                 selectedResultChanged)
  Q_PROPERTY(int selectedFileCursorPosition MEMBER selected_file_cursor_position
                 NOTIFY selectedResultChanged)
  Q_PROPERTY(FileSearchResultFormatter* formatter MEMBER formatter CONSTANT)
 public:
  explicit FindInFilesController(QObject* parent = nullptr);
  ~FindInFilesController();
  QString GetSearchStatus() const;
 public slots:
  void search();
  void selectResult(int i);
  void openSelectedResultInEditor();
 signals:
  void searchTermChanged();
  void searchStatusChanged();
  void optionsChanged();
  void selectedResultChanged();
  void rehighlightBlockByLineNumber(int i);

 private:
  void OnResultFound(QList<FileSearchResult> results);
  void OnSearchComplete();
  void CancelSearchIfRunning();

  QString search_term;
  FindInFilesTask* find_task;
  FileSearchResultListModel* search_results;
  int selected_result;
  QString selected_file_path;
  QString selected_file_content;
  int selected_file_cursor_position;
  FindInFilesOptions options;
  FileSearchResultFormatter* formatter;
};

#endif  // FINDINFILESCONTROLLER_H
