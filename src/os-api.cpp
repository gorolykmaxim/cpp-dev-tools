#include <filesystem>
#include <iostream>
#include <fstream>
#include <memory>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

#include "cdt.h"
#include "process.hpp"

std::istream& OsApi::In() {
    return std::cin;
}

std::ostream& OsApi::Out() {
    return std::cout;
}

std::ostream& OsApi::Err() {
    return std::cerr;
}

std::string OsApi::GetEnv(const std::string &name) {
    const char* value = getenv(name.c_str());
    return value ? value : "";
}

void OsApi::SetCurrentPath(const std::filesystem::path &path) {
    std::filesystem::current_path(path);
}

std::filesystem::path OsApi::GetCurrentPath() {
    return std::filesystem::current_path();
}

std::filesystem::path OsApi::AbsolutePath(const std::filesystem::path &path) {
    return std::filesystem::absolute(path);
}

bool OsApi::ReadFile(const std::filesystem::path &path, std::string &data) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }
    file >> std::noskipws;
    data = std::string(std::istream_iterator<char>(file), std::istream_iterator<char>());
    return true;
}

void OsApi::WriteFile(const std::filesystem::path &path, const std::string &data) {
    std::ofstream file(path);
    file << data;
}

bool OsApi::FileExists(const std::filesystem::path &path) {
    return std::filesystem::exists(path);
}

int OsApi::Exec(const std::vector<const char *> &args) {
    execvp(args[0], const_cast<char* const*>(args.data()));
    return errno;
}

void OsApi::KillProcess(Process& process) {
  if (process.state == ProcessState::kRunning) {
    TinyProcessLib::Process::kill(process.handle->get_id());
  }
}

void OsApi::StartProcess(
    Process& process,
    const std::function<void (const char *, size_t)> &stdout_cb,
    const std::function<void (const char *, size_t)> &stderr_cb,
    const std::function<void ()> &exit_cb) {
  static const std::string err_prefix = "Failed to exec: ";
  static const std::string err_suffix = "\n";
  process.handle = std::make_unique<TinyProcessLib::Process>(
      process.shell_command, "", stdout_cb, stderr_cb, exit_cb);
  if (process.handle->get_id() <= 0) {
    stderr_cb(err_prefix.c_str(), err_prefix.size());
    stderr_cb(process.shell_command.c_str(), process.shell_command.size());
    stderr_cb(err_suffix.c_str(), err_suffix.size());
    exit_cb();
  }
}

int OsApi::GetProcessExitCode(Process& process) {
    return process.handle->get_exit_status();
}

std::chrono::system_clock::time_point OsApi::TimeNow() {
    return std::chrono::system_clock::now();
}
