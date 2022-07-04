#include "blockingconcurrentqueue.h"
#include <filesystem>
#include <functional>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

#include "cdt.h"

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

static void ReadOut(moodycamel::BlockingConcurrentQueue<ProcessEvent>& queue,
                    boost::process::child& process, entt::entity entity,
                    boost::process::ipstream& ipstream, ProcessEventType type) {
  ProcessEvent event;
  event.process = entity;
  event.type = type;
  while (process.running() && ipstream && std::getline(ipstream, event.data)) {
    queue.enqueue(event);
  }
}

static void Exit(moodycamel::BlockingConcurrentQueue<ProcessEvent>& queue,
                 entt::entity entity) {
  ProcessEvent event;
  event.process = entity;
  event.type = ProcessEventType::kExit;
  queue.enqueue(event);
}

bool OsApi::StartProcess(
    entt::entity entity, entt::registry& registry,
    moodycamel::BlockingConcurrentQueue<ProcessEvent>& queue,
    std::string& error) {
  try {
    Process& proc = registry.get<Process>(entity);
    BoostProcess& boost_proc = registry.emplace<BoostProcess>(entity);
    boost_proc.child = std::make_unique<boost::process::child>(
        proc.shell_command, boost::process::std_out > boost_proc.ip_stdout,
        boost::process::std_err > boost_proc.ip_stderr);
    proc.id = boost_proc.child->id();
    boost_proc.read_stdout = std::thread([&boost_proc, &queue, entity] () {
      ReadOut(queue, *boost_proc.child, entity, boost_proc.ip_stdout,
              ProcessEventType::kStdout);
      boost_proc.child->wait();
      Exit(queue, entity);
    });
    boost_proc.read_stderr = std::thread([&boost_proc, &queue, entity] () {
      ReadOut(queue, *boost_proc.child, entity, boost_proc.ip_stderr,
              ProcessEventType::kStderr);
    });
    OnProcessStart(boost_proc);
    return true;
  } catch (const boost::process::process_error& e) {
    error = e.what();
    Exit(queue, entity);
    return false;
  }
}

void OsApi::FinishProcess(entt::entity entity, entt::registry &registry) {
  Process& process = registry.get<Process>(entity);
  BoostProcess& boost_proc = registry.get<BoostProcess>(entity);
  if (boost_proc.child) {
    process.exit_code = boost_proc.child->exit_code();
    boost_proc.read_stdout.join();
    boost_proc.read_stderr.join();
    OnProcessFinish(boost_proc);
  }
  registry.erase<BoostProcess>(entity);
}

std::chrono::system_clock::time_point OsApi::TimeNow() {
    return std::chrono::system_clock::now();
}
