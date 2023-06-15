#ifndef FINDINFILESCONTROLLER_H
#define FINDINFILESCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "syntax.h"
#include "text_list_model.h"

struct FileSearchResult {
  QString match;
  QString file_path;
  int column = -1;
  int row = -1;
  int offset = -1;
  int match_length = -1;
};

class FileSearchResultListModel : public TextListModel {
 public:
  explicit FileSearchResultListModel(QObject* parent);
  void Clear();
  void Append(const QList<FileSearchResult>& items);
  int CountUniqueFiles() const;
  const FileSearchResult& At(int i) const;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;

 private:
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
  Q_PROPERTY(bool expanded MEMBER expanded)
 public:
  bool match_case = false;
  bool match_whole_word = false;
  bool regexp = false;
  bool include_external_search_folders = false;
  QString files_to_include;
  QString files_to_exclude;
  bool exclude_git_ignored_files = true;
  bool expanded = false;

  bool operator==(const FindInFilesOptions& another) const;
  bool operator!=(const FindInFilesOptions& another) const;
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
  Q_PROPERTY(SyntaxFormatter* formatter MEMBER formatter CONSTANT)
 public:
  explicit FindInFilesController(QObject* parent = nullptr);
  ~FindInFilesController();
  QString GetSearchStatus() const;
 public slots:
  void search();
  void openSelectedResultInEditor();
 signals:
  void searchTermChanged();
  void searchStatusChanged();
  void optionsChanged();
  void selectedResultChanged();

 private:
  void OnResultFound(int i);
  void OnSearchComplete();
  void OnSelectedResultChanged();
  void SaveSearchTermAndOptions();

  QString search_term;
  FileSearchResultListModel* search_results;
  QString selected_file_path;
  QString selected_file_content;
  int selected_file_cursor_position;
  FindInFilesOptions options;
  SyntaxFormatter* formatter;
  QFutureWatcher<QList<FileSearchResult>> search_result_watcher;
};

#endif  // FINDINFILESCONTROLLER_H
