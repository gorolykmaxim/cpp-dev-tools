#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>
#include <QtQmlIntegration>

class SettingsController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString openInEditorCommand MEMBER open_in_editor_command NOTIFY
                 settingsChanged)
 public:
  explicit SettingsController(QObject* parent = nullptr);

 public slots:
  void save();

 signals:
  void settingsChanged();

 private:
  void Load();

  QString open_in_editor_command;
};

#endif  // SETTINGSCONTROLLER_H
