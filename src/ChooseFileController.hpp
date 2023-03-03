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
  explicit FileSuggestionListModel(QObject* parent);

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
  void SetPath(const QString& path);
  QString GetPath() const;

 public slots:
  void PickSuggestion(int i);
  void OpenOrCreateFile();
  void CreateFile();

 signals:
  void pathChanged();
  void fileChosen(const QString& path);

 private:
  void SortAndFilterSuggestions();
  QString GetResultPath() const;

  QString folder, file;
  FileSuggestionListModel* suggestions;
};
