#include "UserInterface.hpp"

static const QString kCurrentView = "currentView";

InputAndListView::InputAndListView(QQmlContext* context) : context(context) {
  context->setContextProperty("inputAndListData", this);
}

void InputAndListView::Display(const QString& title) {
  this->title = title;
  emit TitleChanged();
  context->setContextProperty(kCurrentView, "InputAndListView.qml");
}

UserInterface::UserInterface(QQmlContext* context) : input_and_list(context) {
  context->setContextProperty(kCurrentView, "");
}
