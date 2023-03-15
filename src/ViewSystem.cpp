#include "ViewSystem.hpp"

#include <QGuiApplication>
#include <QScreen>

#include "Application.hpp"
#include "Database.hpp"

#define LOG() qDebug() << "[ViewSystem]"

WindowDimensions WindowDimensions::ReadFromSql(QSqlQuery &query) {
  WindowDimensions dimensions;
  dimensions.width = query.value(0).toInt();
  dimensions.height = query.value(1).toInt();
  dimensions.x = query.value(2).toInt();
  dimensions.y = query.value(3).toInt();
  return dimensions;
}

bool WindowDimensions::operator==(const WindowDimensions &another) const {
  return width == another.width && height == another.height && x == another.x &&
         y == another.y;
}

bool WindowDimensions::operator!=(const WindowDimensions &another) const {
  return !(*this == another);
}

QDebug operator<<(QDebug debug, const WindowDimensions &dimensions) {
  QDebugStateSaver saver(debug);
  return debug.nospace() << "WindowDimensions(width=" << dimensions.width
                         << ",height=" << dimensions.height
                         << ",x=" << dimensions.x << ",y=" << dimensions.y
                         << ')';
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
  screen_size = QGuiApplication::primaryScreen()->availableSize();
  SetDefaultWindowSize();
  LOG() << "Loading window dimensions from database";
  QFuture<QList<WindowDimensions>> future =
      QtConcurrent::run(&Application::Get().io_thread_pool, [this] {
        return Database::ExecQueryAndRead<WindowDimensions>(
            "SELECT width, height, x, y FROM window_dimensions WHERE "
            "screen_width=? AND screen_height=?",
            &WindowDimensions::ReadFromSql,
            {screen_size.width(), screen_size.height()});
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
  dimensions = WindowDimensions();
  dimensions.x = screen_size.width() / 2 - dimensions.width / 2;
  dimensions.y = screen_size.height() / 2 - dimensions.height / 2;
  emit windowDimensionsChanaged();
}

void ViewSystem::SaveWindowDimensions(int width, int height, int x,
                                      int y) const {
  if (x < 0) {
    x = 0;
  }
  if (y < 0) {
    y = 0;
  }
  LOG() << "Saving new window dimensions:" << width << height << x << y;
  Database::ExecCmdAsync(
      "INSERT OR REPLACE INTO window_dimensions VALUES(?, ?, ?, ?, ?, ?)",
      {screen_size.width(), screen_size.height(), width, height, x, y});
}
