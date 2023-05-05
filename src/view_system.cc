#include "view_system.h"

#include <QGuiApplication>
#include <QScreen>

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

void ViewSystem::DisplayAlertDialog(AlertDialog dialog) {
  emit dialogClosed();
  LOG() << "Displaying" << dialog;
  QObject::disconnect(this, &ViewSystem::alertDialogAccepted, nullptr, nullptr);
  QObject::disconnect(this, &ViewSystem::alertDialogRejected, nullptr, nullptr);
  if (dialog.flags & AlertDialog::kError) {
    dialog.flags &= ~AlertDialog::kCancellable;
  }
  if (dialog.text.count('\n') > 1) {
    dialog.flags |= AlertDialog::kFullHeight;
  }
  emit alertDialogDisplayed(dialog);
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

void ViewSystem::Initialize() {
  DetermineWindowDimensions();
  LoadSplitViewStates();
}

void ViewSystem::SetDefaultWindowSize() {
  QSize screen_size = QGuiApplication::primaryScreen()->availableSize();
  dimensions = WindowDimensions();
  dimensions.x = screen_size.width() / 2 - dimensions.width / 2;
  dimensions.y = screen_size.height() / 2 - dimensions.height / 2;
  LOG() << "Reseting to default" << dimensions;
  emit windowDimensionsChanaged();
}

void ViewSystem::saveWindowDimensions(int width, int height, int x, int y,
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

void ViewSystem::saveSplitViewState(const QString &id,
                                    const QByteArray &state) {
  LOG() << "State of" << id << "split view changed";
  QRect virtual_desktop = QGuiApplication::primaryScreen()->virtualGeometry();
  split_view_states[id] = state;
  Database::ExecCmdAsync(
      "INSERT OR REPLACE INTO split_view_state VALUES(?, ?, ?, ?, ?, ?)",
      {id, virtual_desktop.width(), virtual_desktop.height(),
       virtual_desktop.x(), virtual_desktop.y(), state});
}

QByteArray ViewSystem::getSplitViewState(const QString &id) {
  return split_view_states[id];
}

void ViewSystem::DetermineWindowDimensions() {
  QRect virtual_desktop = QGuiApplication::primaryScreen()->virtualGeometry();
  SetDefaultWindowSize();
  LOG() << "Loading window dimensions from database";
  QList<WindowDimensions> results =
      Database::ExecQueryAndReadSync<WindowDimensions>(
          "SELECT width, height, x, y, is_maximized FROM window_dimensions "
          "WHERE virtual_width=? AND virtual_height=? AND virtual_x=? AND "
          "virtual_y=?",
          &WindowDimensions::ReadFromSql,
          {virtual_desktop.width(), virtual_desktop.height(),
           virtual_desktop.x(), virtual_desktop.y()});
  if (!results.isEmpty()) {
    dimensions = results[0];
    LOG() << "Will use existing" << dimensions;
  } else {
    LOG() << "No existing window dimensions found - will fallback to the "
             "default one";
  }
}

struct SplitViewState {
  QString id;
  QByteArray state;

  static SplitViewState ReadFromSql(QSqlQuery &sql) {
    SplitViewState state;
    state.id = sql.value(0).toString();
    state.state = sql.value(1).toByteArray();
    return state;
  }
};

void ViewSystem::LoadSplitViewStates() {
  LOG() << "Loading split view states from the database";
  split_view_states.clear();
  QRect virtual_desktop = QGuiApplication::primaryScreen()->virtualGeometry();
  QList<SplitViewState> states = Database::ExecQueryAndReadSync<SplitViewState>(
      "SELECT id, state FROM split_view_state WHERE virtual_width=? AND "
      "virtual_height=? AND virtual_x=? AND virtual_y=?",
      &SplitViewState::ReadFromSql,
      {virtual_desktop.width(), virtual_desktop.height(), virtual_desktop.x(),
       virtual_desktop.y()});
  for (const SplitViewState &state : states) {
    split_view_states[state.id] = state.state;
  }
  LOG() << "Loaded split view states:" << split_view_states.keys();
}

AlertDialog::AlertDialog(const QString &title, const QString &text)
    : title(title), text(text), flags(kCancellable) {}

bool AlertDialog::operator==(const AlertDialog &another) const {
  return title == another.title && text == another.text &&
         flags == another.flags;
}

bool AlertDialog::operator!=(const AlertDialog &another) const {
  return !(*this == another);
}

bool AlertDialog::IsError() const { return flags & kError; }

bool AlertDialog::IsCancellable() const { return flags & kCancellable; }

bool AlertDialog::IsFullHeight() const { return flags & kFullHeight; }

QDebug operator<<(QDebug debug, const AlertDialog &dialog) {
  QDebugStateSaver saver(debug);
  return debug.nospace() << "AlertDialog(title=" << dialog.title
                         << ",flags=" << dialog.flags << ')';
}
