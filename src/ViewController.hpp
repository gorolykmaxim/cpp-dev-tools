#pragma once

#include <QObject>
#include <QtQmlIntegration>

class ViewController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString currentView READ GetCurrentView WRITE SetCurrentView NOTIFY
                 currentViewChanged)
 public:
  void SetCurrentView(const QString& current_view);
  QString GetCurrentView() const;

 signals:
  void currentViewChanged();

 private:
  QString current_view;
};
