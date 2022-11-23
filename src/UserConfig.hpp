#pragma once

#include "AppData.hpp"

void ReadUserConfigFrom(AppData& app, const QJsonDocument& json);
void SaveToUserConfig(AppData& app);
