#include "UserInterface.hpp"
#include "Common.hpp"

UserInterface::UserInterface(QQmlContext* context) : input_and_list(context) {
  context->setContextProperty(kQmlCurrentView, "");
}
