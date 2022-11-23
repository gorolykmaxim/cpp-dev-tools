#pragma once

#include "Application.hpp"

void ReadUserConfigFrom(Application& app, const QJsonDocument& json);
void SaveToUserConfig(Application& app);
