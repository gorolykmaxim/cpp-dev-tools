#ifndef FINDINFILESCONTROLLER_H
#define FINDINFILESCONTROLLER_H

#include <QAbstractListModel>
#include <QObject>
#include <QtQmlIntegration>

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

class FindInFilesTask : public BaseIoTask {
  Q_OBJECT
 public:
  FindInFilesTask(QObject* parent, const QString& search_term);

 signals:
  void resultsFound(QList<FileSearchResult> result);

 protected:
  void RunInBackground() override;

 private:
  QString search_term;
  QString folder;
};

class FindInFilesController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString searchTerm MEMBER search_term NOTIFY searchTermChanged)
  Q_PROPERTY(FileSearchResultListModel* results MEMBER search_results CONSTANT)
  Q_PROPERTY(
      QString searchStatus READ GetSearchStatus NOTIFY searchStatusChanged)
 public:
  explicit FindInFilesController(QObject* parent = nullptr);
  QString GetSearchStatus() const;
 public slots:
  void search();
  void selectResult(int i);
  void openSelectedResultInEditor();
 signals:
  void searchTermChanged();
  void searchStatusChanged();

 private:
  void OnResultFound(QList<FileSearchResult> results);
  void OnSearchComplete();

  QString search_term;
  FindInFilesTask* find_task;
  FileSearchResultListModel* search_results;
  int selected_result;
};

#endif  // FINDINFILESCONTROLLER_H
