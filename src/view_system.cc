#include "view_system.h"

#include <QGuiApplication>
#include <QScreen>

#include "application.h"
#include "database.h"

#define LOG() qDebug() << "[ViewSystem]"

WindowDimensions WindowDimensions::ReadFromSql(QSqlQuery &query) {
  WindowDimensions dimensions;
  dimensions.width = query.value(0).toInt();
  dimensions.height = query.value(1).toInt();
  dimensions.x = query.value(2).toInt();
  dimensions.y = query.value(3).toInt();
  dimensions.is_maximized = query.value(4).toBool();
  return dimensions;
}

bool WindowDimensions::operator==(const WindowDimensions &another) const {
  return width == another.width && height == another.height && x == another.x &&
         y == another.y && is_maximized == another.is_maximized;
}

bool WindowDimensions::operator!=(const WindowDimensions &another) const {
  return !(*this == another);
}

QDebug operator<<(QDebug debug, const WindowDimensions &dimensions) {
  QDebugStateSaver saver(debug);
  return debug.nospace() << "WindowDimensions(width=" << dimensions.width
                         << ",height=" << dimensions.height
                         << ",x=" << dimensions.x << ",y=" << dimensions.y
                         << ",is_maximized=" << dimensions.is_maximized << ')';
}

void ViewSystem::SetCurrentView(const QString &current_view) {
  emit dialogClosed();
  LOG() << "Changing current view to" << current_view;
  this->current_view = "";
  emit currentViewChanged();
  this->current_view = current_view;
  emit currentViewChanged();
}

QString ViewSystem::GetCurrentView() const { return current_view; }

void ViewSystem::DisplayAlertDialog(const QString &title, const QString &text) {
  emit dialogClosed();
  LOG() << "Displaying alert dialog";
  QObject::disconnect(this, &ViewSystem::alertDialogAccepted, nullptr, nullptr);
  QObject::disconnect(this, &ViewSystem::alertDialogRejected, nullptr, nullptr);
  emit alertDialogDisplayed(title, text);
}

void ViewSystem::SetWindowTitle(const QString &title) {
  LOG() << "Changing window title to" << title;
  window_title = title;
  emit windowTitleChanged();
}

QString ViewSystem::GetWindowTitle() const { return window_title; }

void ViewSystem::DisplaySearchUserCommandDialog() {
  emit dialogClosed();
  LOG() << "Displaying search user command dialog";
  emit searchUserCommandDialogDisplayed();
}

void ViewSystem::DetermineWindowDimensions() {
  QRect virtual_desktop = QGuiApplication::primaryScreen()->virtualGeometry();
  SetDefaultWindowSize();
  LOG() << "Loading window dimensions from database";
  QFuture<QList<WindowDimensions>> future =
      QtConcurrent::run(&Application::Get().io_thread_pool, [virtual_desktop] {
        return Database::ExecQueryAndRead<WindowDimensions>(
            "SELECT width, height, x, y, is_maximized FROM window_dimensions "
            "WHERE virtual_width=? AND virtual_height=? AND virtual_x=? AND "
            "virtual_y=?",
            &WindowDimensions::ReadFromSql,
            {virtual_desktop.width(), virtual_desktop.height(),
             virtual_desktop.x(), virtual_desktop.y()});
      });
  while (!future.isResultReadyAt(0)) {
    // If we just call result() - Qt will not just block current thread waiting
    // for the result, it will RUN the specified function
    // on the current thread :) which will crash the app because the Database
    // instance only exists on the IO thread. So instead of relying on Qt
    // to block waiting for result, lets block ourselves with this simple
    // spin-lock.
  };
  QList<WindowDimensions> results = future.result();
  if (!results.isEmpty()) {
    dimensions = results[0];
    LOG() << "Will use existing" << dimensions;
  } else {
    LOG() << "No existing window dimensions found - will fallback to the "
             "default one";
  }
}

void ViewSystem::SetDefaultWindowSize() {
  QSize screen_size = QGuiApplication::primaryScreen()->availableSize();
  dimensions = WindowDimensions();
  dimensions.x = screen_size.width() / 2 - dimensions.width / 2;
  dimensions.y = screen_size.height() / 2 - dimensions.height / 2;
  LOG() << "Reseting to default" << dimensions;
  emit windowDimensionsChanaged();
}

void ViewSystem::SaveWindowDimensions(int width, int height, int x, int y,
                                      bool is_maximized) const {
  QRect virtual_desktop = QGuiApplication::primaryScreen()->virtualGeometry();
  LOG() << "Saving new window dimensions:" << width << height << x << y
        << is_maximized;
  Database::ExecCmdAsync(
      "INSERT OR REPLACE INTO window_dimensions VALUES(?, ?, ?, ?, ?, ?, ?, ?, "
      "?)",
      {virtual_desktop.width(), virtual_desktop.height(), virtual_desktop.x(),
       virtual_desktop.y(), width, height, x, y, is_maximized});
}