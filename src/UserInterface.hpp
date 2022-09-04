#pragma once

#include <QQmlContext>
#include "InputAndListView.hpp"

class UserInterface {
public:
  UserInterface(QQmlContext* context);

  InputAndListView input_and_list;
};
