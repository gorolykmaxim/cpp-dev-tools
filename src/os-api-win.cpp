#include <algorithm>
#include <vector>
#include <windows.h>
#include <functional>
#include <mutex>
#include <io.h>
#include <fcntl.h>
#include <codecvt>
#include <iostream>

#include "cdt.h"
#include "process.hpp"

static std::vector<TinyProcessLib::Process::id_type> active_process_ids;
static std::mutex active_process_ids_mtx;

static bool IsWindowsConsole() {
  DWORD mode;
  return GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode) == 1;
}

std::string OsApi::ReadLineFromStdin() {
  using utf16_to_utf8 = std::codecvt_utf8_utf16<wchar_t>;
  if (IsWindowsConsole()) {
    std::wstring winput;
    std::getline(std::wcin, winput);
    return std::wstring_convert<utf16_to_utf8, wchar_t>{}.to_bytes(winput);
  } else {
    std::string input;
    std::getline(std::cin, input);
    return input;
  }
}

void OsApi::SetEnv(const std::string &name, const std::string &value) {
    SetEnvironmentVariable(name.c_str(), value.c_str());
}

std::filesystem::path OsApi::GetHome() {
  return GetEnv("HOMEDRIVE") + GetEnv("HOMEPATH");
}

BOOL WINAPI HandleCtrlC(DWORD signal) {
  if (signal != CTRL_C_EVENT) {
    return FALSE;
  }
  std::lock_guard<std::mutex> lock(active_process_ids_mtx);
  for (TinyProcessLib::Process::id_type id: active_process_ids) {
    TinyProcessLib::Process::kill(id);
  }
  return active_process_ids.empty() ? FALSE : TRUE;
}

void OsApi::Init() {
  if (IsWindowsConsole()) {
    _setmode(_fileno(stdin), _O_U16TEXT);
    SetConsoleOutputCP(CP_UTF8);
  }
  SetConsoleCtrlHandler(HandleCtrlC, TRUE);
}

void OsApi::StartProcess(
    Process &process,
    const std::function<void (const char *, size_t)> &stdout_cb,
    const std::function<void (const char *, size_t)> &stderr_cb,
    const std::function<void ()> &exit_cb) {
  process.handle = std::make_unique<TinyProcessLib::Process>(
      process.shell_command, "", stdout_cb, stderr_cb, exit_cb);
  process.id = process.handle->get_id();
  std::lock_guard<std::mutex> lock(active_process_ids_mtx);
  active_process_ids.push_back(process.handle->get_id());
}

void OsApi::FinishProcess(Process& process) {
  std::lock_guard<std::mutex> lock(active_process_ids_mtx);
  auto it = std::find(active_process_ids.begin(), active_process_ids.end(),
                      process.handle->get_id());
  if (it != active_process_ids.end()) {
    active_process_ids.erase(it);
  }
}
