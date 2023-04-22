#ifndef FINDINFILESCONTROLLER_H
#define FINDINFILESCONTROLLER_H

#include <QAbstractListModel>
#include <QObject>
#include <QtQmlIntegration>
#include <atomic>

#include "io_task.h"

struct FileSearchResult {
  QString match;
  QString file_path;
  int column = -1;
  int row = -1;
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
 public:
  bool match_case = false;
  bool match_whole_word = false;
  bool regexp = false;
  bool include_external_search_folders = false;

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
  QList<FileSearchResult> Find(const QString& line, int column,
                               const QString& file_name) const;
  QList<FileSearchResult> FindRegex(const QString& line, int column,
                                    const QString& file_name) const;

  QString search_term;
  QRegularExpression search_term_regex;
  QString folder;
  FindInFilesOptions options;
};

class FindInFilesController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString searchTerm MEMBER search_term NOTIFY searchTermChanged)
  Q_PROPERTY(FileSearchResultListModel* results MEMBER search_results CONSTANT)
  Q_PROPERTY(
      QString searchStatus READ GetSearchStatus NOTIFY searchStatusChanged)
  Q_PROPERTY(FindInFilesOptions options MEMBER options NOTIFY optionsChanged)
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

 private:
  void OnResultFound(QList<FileSearchResult> results);
  void OnSearchComplete();
  void CancelSearchIfRunning();

  QString search_term;
  FindInFilesTask* find_task;
  FileSearchResultListModel* search_results;
  int selected_result;
  FindInFilesOptions options;
};

#endif  // FINDINFILESCONTROLLER_H
