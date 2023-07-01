#ifndef RUNCMAKECONTROLLER_H
#define RUNCMAKECONTROLLER_H

#include <QObject>
#include <QQmlEngine>

class RunCmakeController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString sourceFolder MEMBER source_folder NOTIFY foldersChanged)
  Q_PROPERTY(QString buildFolder MEMBER build_folder NOTIFY foldersChanged)
  Q_PROPERTY(QString rootFolder READ GetRootFolder CONSTANT)
 public:
  explicit RunCmakeController(QObject* parent = nullptr);
  QString GetRootFolder() const;

 public slots:
  void displayChooseSourceFolder();
  void displayChooseBuildFolder();
  void setBuildFolder(QString path);
  void setSourceFolder(QString path);
  void back();
  void runCmake();

 signals:
  void foldersChanged();
  void displayView();
  void displayChooseBuildFolderView();
  void displayChooseSourceFolderView();

 private:
  void SaveState() const;

  QString source_folder;
  QString build_folder;
};

#endif  // RUNCMAKECONTROLLER_H
