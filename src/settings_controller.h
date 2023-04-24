#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "qvariant_list_model.h"

class FolderListModel : public QVariantListModel {
 public:
  explicit FolderListModel(QObject* parent);

  QStringList list;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;
};

struct Settings {
  Q_GADGET
  Q_PROPERTY(QString openInEditorCommand MEMBER open_in_editor_command)
  Q_PROPERTY(int taskHistoryLimit MEMBER task_history_limit)
 public:
  bool operator==(const Settings& another) const;
  bool operator!=(const Settings& another) const;

  QString open_in_editor_command;
  int task_history_limit;
  QStringList external_search_folders;
};

class SettingsController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(Settings settings MEMBER settings NOTIFY settingsChanged)
  Q_PROPERTY(FolderListModel* externalSearchFolders MEMBER
                 external_search_folders CONSTANT)
 public:
  explicit SettingsController(QObject* parent = nullptr);

 public slots:
  void configureExternalSearchFolders();
  void addExternalSearchFolder(const QString& folder);
  void removeExternalSearchFolder(const QString& folder);
  void goToSettings();
  void save();

 signals:
  void settingsChanged();
  void openExternalSearchFoldersEditor();
  void openSettings();

 private:
  void Load();

  Settings settings;
  FolderListModel* external_search_folders;
};

#endif  // SETTINGSCONTROLLER_H
