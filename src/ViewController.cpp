#include "ViewController.hpp"

#include <QDebug>

#define LOG() qDebug() << "[ViewController]"

void ViewController::SetCurrentView(const QString &current_view) {
  emit dialogClosed();
  LOG() << "Changing current view to" << current_view;
  this->current_view = "";
  emit currentViewChanged();
  this->current_view = current_view;
  emit currentViewChanged();
}

QString ViewController::GetCurrentView() const { return current_view; }

void ViewController::DisplayAlertDialog(const QString &title,
                                        const QString &text) {
  emit dialogClosed();
  LOG() << "Displaying alert dialog";
  QObject::disconnect(this, &ViewController::alertDialogAccepted, nullptr,
                      nullptr);
  QObject::disconnect(this, &ViewController::alertDialogRejected, nullptr,
                      nullptr);
  emit alertDialogDisplayed(title, text);
}

void ViewController::DisplaySearchUserCommandDialog() {
  emit dialogClosed();
  LOG() << "Displaying search user command dialog";
  emit searchUserCommandDialogDisplayed();
}
