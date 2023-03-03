#pragma once

#include <QObject>

class ViewController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString currentView READ GetCurrentView WRITE SetCurrentView NOTIFY
                 currentViewChanged)
 public:
  void SetCurrentView(const QString& current_view);
  QString GetCurrentView() const;
  void DisplayAlertDialog(const QString& title, const QString& text);

 public slots:
  void DisplaySearchUserCommandDialog();

 signals:
  void currentViewChanged();
  void alertDialogDisplayed(const QString& title, const QString& text);
  void alertDialogAccepted();
  void alertDialogRejected();
  void searchUserCommandDialogDisplayed();

 private:
  QString current_view = "SelectProject.qml";
};
