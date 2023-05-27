#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

#include "database.h"
#include "text_list_model.h"
#include "ui_icon.h"

class FolderListModel : public TextListModel {
  Q_OBJECT
 public:
  explicit FolderListModel(QObject* parent, const QString& table,
                           const UiIcon& icon);
  void SetFolders(QStringList& folders);
  QList<Database::Cmd> MakeCommandsToUpdateDatabase();

  QStringList list;

 public slots:
  void addFolder(const QString& folder);
  void removeFolder(const QString& folder);

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;

 private:
  QString table;
  UiIcon icon;
};

class TerminalListModel : public TextListModel {
 public:
  explicit TerminalListModel(QObject* parent);

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
  QStringList documentation_folders;
  QStringList terminals;
};

class SettingsController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(Settings settings MEMBER settings NOTIFY settingsChanged)
  Q_PROPERTY(FolderListModel* externalSearchFolders MEMBER
                 external_search_folders CONSTANT)
  Q_PROPERTY(FolderListModel* documentationFolders MEMBER documentation_folders
                 CONSTANT)
  Q_PROPERTY(TerminalListModel* terminals MEMBER terminals CONSTANT)
 public:
  explicit SettingsController(QObject* parent = nullptr);

 public slots:
  void configureExternalSearchFolders();
  void configureDocumentationFolders();
  void goToSettings();
  void save();
  void moveSelectedTerminal(bool up);

 signals:
  void settingsChanged();
  void openExternalSearchFoldersEditor();
  void openDocumentationFoldersEditor();
  void openSettings();

 private:
  void Load();

  Settings settings;
  FolderListModel* external_search_folders;
  FolderListModel* documentation_folders;
  TerminalListModel* terminals;
};

#endif  // SETTINGSCONTROLLER_H
