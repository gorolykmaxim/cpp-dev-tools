#include "ViewSystem.hpp"

#include <QDebug>

#define LOG() qDebug() << "[ViewSystem]"

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
