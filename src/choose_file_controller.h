#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "qvariant_list_model.h"

struct FileSuggestion {
  QString name;
  bool is_dir = false;
};

class FileSuggestionListModel : public QVariantListModel {
 public:
  explicit FileSuggestionListModel(QObject* parent);
  int IndexOfSuggestionWithName(const QString& name) const;

  QList<FileSuggestion> list;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;
};

class ChooseFileController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString path READ GetPath WRITE SetPath NOTIFY pathChanged)
  Q_PROPERTY(FileSuggestionListModel* suggestions MEMBER suggestions CONSTANT)
  Q_PROPERTY(
      bool allowCreating MEMBER allow_creating NOTIFY allowCreatingChanged)
  Q_PROPERTY(bool canOpen READ CanOpen NOTIFY pathChanged)
  Q_PROPERTY(bool chooseFolder MEMBER choose_folder NOTIFY chooseFolderChanged)
 public:
  explicit ChooseFileController(QObject* parent = nullptr);
  void SetPath(const QString& path);
  QString GetPath() const;
  bool CanOpen() const;

 public slots:
  void pickSuggestion(int i);
  void openOrCreateFile();

 signals:
  void pathChanged();
  void fileChosen(const QString& path);
  void allowCreatingChanged();
  void chooseFolderChanged();

 private:
  QString GetResultPath() const;
  void CreateFile();

  QString folder, file;
  FileSuggestionListModel* suggestions;
  bool allow_creating = true;
  bool choose_folder = false;
};
