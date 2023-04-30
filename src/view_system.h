#pragma once

#include <QDebug>
#include <QObject>
#include <QSqlQuery>

struct WindowDimensions {
  Q_GADGET
  Q_PROPERTY(int width MEMBER width CONSTANT)
  Q_PROPERTY(int height MEMBER height CONSTANT)
  Q_PROPERTY(int x MEMBER x CONSTANT)
  Q_PROPERTY(int y MEMBER y CONSTANT)
  Q_PROPERTY(bool isMaximized MEMBER is_maximized CONSTANT)
 public:
  int width = 1024;
  int height = 600;
  int x = 0;
  int y = 0;
  bool is_maximized = false;

  static WindowDimensions ReadFromSql(QSqlQuery& query);
  bool operator==(const WindowDimensions& another) const;
  bool operator!=(const WindowDimensions& another) const;
};

QDebug operator<<(QDebug debug, const WindowDimensions& dimensions);

class ViewSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString currentView READ GetCurrentView WRITE SetCurrentView NOTIFY
                 currentViewChanged)
  Q_PROPERTY(QString windowTitle READ GetWindowTitle WRITE SetWindowTitle NOTIFY
                 windowTitleChanged)
  Q_PROPERTY(WindowDimensions dimensions MEMBER dimensions NOTIFY
                 windowDimensionsChanaged)
 public:
  void SetCurrentView(const QString& current_view);
  QString GetCurrentView() const;
  void DisplayAlertDialog(const QString& title, const QString& text,
                          bool is_error = false);
  void SetWindowTitle(const QString& title);
  QString GetWindowTitle() const;
  void Initialize();
  void SetDefaultWindowSize();

 public slots:
  void DisplaySearchUserCommandDialog();
  void saveWindowDimensions(int width, int height, int x, int y,
                            bool is_maximized) const;
  void saveSplitViewState(const QString& id, const QByteArray& state);
  QByteArray getSplitViewState(const QString& id);

 signals:
  void currentViewChanged();
  void alertDialogDisplayed(const QString& title, const QString& text,
                            bool is_error);
  void alertDialogAccepted();
  void alertDialogRejected();
  void searchUserCommandDialogDisplayed();
  void dialogClosed();
  void windowTitleChanged();
  void windowDimensionsChanaged();

 private:
  void DetermineWindowDimensions();
  void LoadSplitViewStates();

  QString current_view = "SelectProject.qml";
  QString window_title = "CPP Dev Tools";
  WindowDimensions dimensions;
  QHash<QString, QByteArray> split_view_states;
};
