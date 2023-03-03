#pragma once

#include <QObject>

class ViewController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString currentView READ GetCurrentView WRITE SetCurrentView NOTIFY
                 currentViewChanged)
 public:
  void SetCurrentView(const QString& current_view);
  QString GetCurrentView() const;

 signals:
  void currentViewChanged();

 private:
  QString current_view = "SelectProject.qml";
};
