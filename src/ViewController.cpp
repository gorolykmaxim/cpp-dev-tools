#include "ViewController.hpp"

void ViewController::SetCurrentView(const QString &current_view) {
  this->current_view = "";
  emit currentViewChanged();
  this->current_view = current_view;
  emit currentViewChanged();
}

QString ViewController::GetCurrentView() const { return current_view; }
