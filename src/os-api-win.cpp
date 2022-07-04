#include <algorithm>
#include <vector>
#include <boost/process.hpp>
#include <windows.h>
#include <functional>
#include <mutex>
#include <io.h>
#include <fcntl.h>
#include <codecvt>
#include <iostream>

#include "cdt.h"

static std::vector<boost::process::child*> active_processes;
static std::mutex active_processes_mtx;

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
  std::lock_guard<std::mutex> lock(active_processes_mtx);
  for (boost::process::child* proc: active_processes) {
    proc->terminate();
  }
  return active_processes.empty() ? FALSE : TRUE;
}

void OsApi::Init() {
  if (IsWindowsConsole()) {
    _setmode(_fileno(stdin), _O_U16TEXT);
    SetConsoleOutputCP(CP_UTF8);
  }
  SetConsoleCtrlHandler(HandleCtrlC, TRUE);
}

void OsApi::OnProcessStart(BoostProcess& process) {
  std::lock_guard<std::mutex> lock(active_processes_mtx);
  active_processes.push_back(process.child.get());
}

void OsApi::OnProcessFinish(BoostProcess& process) {
  std::lock_guard<std::mutex> lock(active_processes_mtx);
  auto it = std::find(active_processes.begin(), active_processes.end(),
                      process.child.get());
  if (it != active_processes.end()) {
    active_processes.erase(it);
  }
}
