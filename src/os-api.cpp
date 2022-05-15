#include <filesystem>
#include <iostream>
#include <fstream>
#include <memory>
#include <unistd.h>
#include <csignal>

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

void OsApi::SetEnv(const std::string &name, const std::string &value) {
    setenv(name.c_str(), value.c_str(), true);
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

void OsApi::Signal(int signal, void (*handler)(int)) {
    std::signal(signal, handler);
}

void OsApi::RaiseSignal(int signal) {
    std::raise(signal);
}

int OsApi::Exec(const std::vector<const char *> &args) {
    execvp(args[0], const_cast<char* const*>(args.data()));
    return errno;
}

void OsApi::KillProcess(Process& process) {
    TinyProcessLib::Process::kill(process.handle->get_id());
}

void OsApi::ExecProcess(const std::string &shell_cmd) {
    TinyProcessLib::Process(shell_cmd).get_exit_status();
}

void OsApi::StartProcess(Process& process,
                         const std::function<void (const char *, size_t)> &stdout_cb,
                         const std::function<void (const char *, size_t)> &stderr_cb,
                         const std::function<void ()> &exit_cb) {
    process.handle = std::make_unique<TinyProcessLib::Process>(process.shell_command, "", stdout_cb, stderr_cb, exit_cb);
}

int OsApi::GetProcessExitCode(Process& process) {
    return process.handle->get_exit_status();
}

std::chrono::system_clock::time_point OsApi::TimeNow() {
    return std::chrono::system_clock::now();
}
