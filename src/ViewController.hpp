#pragma once

#include <QObject>

class ViewController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString currentView READ GetCurrentView WRITE SetCurrentView NOTIFY
                 currentViewChanged)
  Q_PROPERTY(QString windowTitle READ GetWindowTitle WRITE SetWindowTitle NOTIFY
                 windowTitleChanged)
 public:
  void SetCurrentView(const QString& current_view);
  QString GetCurrentView() const;
  void DisplayAlertDialog(const QString& title, const QString& text);
  void SetWindowTitle(const QString& title);
  QString GetWindowTitle() const;

 public slots:
  void DisplaySearchUserCommandDialog();

 signals:
  void currentViewChanged();
  void alertDialogDisplayed(const QString& title, const QString& text);
  void alertDialogAccepted();
  void alertDialogRejected();
  void searchUserCommandDialogDisplayed();
  void dialogClosed();
  void windowTitleChanged();

 private:
  QString current_view = "SelectProject.qml";
  QString window_title = "CPP Dev Tools";
};
