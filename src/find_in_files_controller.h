#ifndef FINDINFILESCONTROLLER_H
#define FINDINFILESCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "io_task.h"
#include "qvariant_list_model.h"

struct FileSearchResult {
  QString match;
  QString file_path;
  int column = -1;
  int row = -1;
};

class FileSearchResultListModel : public QVariantListModel {
 public:
  explicit FileSearchResultListModel(QObject* parent);

  QList<FileSearchResult> list;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;
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
 signals:
  void searchTermChanged();
  void searchStatusChanged();

 private:
  void OnResultFound(QList<FileSearchResult> results);
  void OnSearchComplete();

  QString search_term;
  FindInFilesTask* find_task;
  FileSearchResultListModel* search_results;
};

#endif  // FINDINFILESCONTROLLER_H
