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

struct AlertDialog {
  Q_GADGET
  Q_PROPERTY(QString title MEMBER title CONSTANT)
  Q_PROPERTY(QString text MEMBER text CONSTANT)
  Q_PROPERTY(bool isError READ IsError CONSTANT)
  Q_PROPERTY(bool isCancellable READ IsCancellable CONSTANT)
  Q_PROPERTY(bool isFullHeight READ IsFullHeight CONSTANT)
 public:
  enum Flags {
    kNoFlags = 0,
    kError = 1,
    kCancellable = 2,
    kFullHeight = 4,
  };
  AlertDialog(const QString& title, const QString& text);
  bool operator==(const AlertDialog& another) const;
  bool operator!=(const AlertDialog& another) const;
  bool IsError() const;
  bool IsCancellable() const;
  bool IsFullHeight() const;

  QString title;
  QString text;
  int flags = kCancellable;
};

QDebug operator<<(QDebug debug, const AlertDialog& dialog);

class ViewSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString currentView READ GetCurrentView WRITE SetCurrentView NOTIFY
                 currentViewChanged)
  Q_PROPERTY(QString windowTitle READ GetWindowTitle WRITE SetWindowTitle NOTIFY
                 windowTitleChanged)
  Q_PROPERTY(WindowDimensions dimensions MEMBER dimensions NOTIFY
                 windowDimensionsChanaged)
 public:
  static QBrush BrushFromHex(const QString& hex);
  static int CalcWidthInMonoFont(const QString& str);
  void SetCurrentView(const QString& current_view);
  QString GetCurrentView() const;
  void DisplayAlertDialog(AlertDialog dialog);
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
  void alertDialogDisplayed(const AlertDialog& dialog);
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
