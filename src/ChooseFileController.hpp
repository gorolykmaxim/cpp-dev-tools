#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "QVariantListModel.hpp"

class FileSuggestion {
 public:
  QString file;
  int match_start;
  int distance;
};

class FileSuggestionListModel : public QVariantListModel {
 public:
  FileSuggestionListModel();

  QList<FileSuggestion> list;

 protected:
  QVariantList GetRow(int i) const override;
  int GetRowCount() const override;
};

class ChooseFileController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString path READ GetPath WRITE SetPath NOTIFY pathChanged)
  Q_PROPERTY(FileSuggestionListModel* suggestions MEMBER suggestions CONSTANT)
 public:
  explicit ChooseFileController(QObject* parent = nullptr);
  ~ChooseFileController();
  void SetPath(const QString& path);
  QString GetPath() const;

 public slots:
  void PickSuggestion(int i);
  void OpenOrCreateFile();

 signals:
  void pathChanged();
  void willCreateFile();
  void fileChosen();

 private:
  void SortAndFilterSuggestions();

  QString folder, file;
  FileSuggestionListModel* suggestions;
};
