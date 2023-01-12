#pragma once

#include "AppData.hpp"

Exec* FindExecById(AppData& app, QUuid id);
Exec* FindLatestRunningExec(AppData& app);
