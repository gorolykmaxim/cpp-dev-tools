#ifndef CDT_H
#define CDT_H

#include <chrono>
#include <cstddef>
#include <functional>
#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>
#include <optional>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include "process.hpp"
#include "blockingconcurrentqueue.h"
#include "entt.hpp"

const std::string kTcRed = "\033[31m";
const std::string kTcGreen = "\033[32m";
const std::string kTcBlue = "\033[34m";
const std::string kTcMagenta = "\033[35m";
const std::string kTcReset = "\033[0m";

const std::string kOpenInEditorCommandProperty = "open_in_editor_command";
const std::string kDebugCommandProperty = "debug_command";
const std::string kEnvVarLastCommand = "LAST_COMMAND";
const std::string kGtestTask = "__gtest";
const std::string kGtestFilterArg = "--gtest_filter";

struct UserCommandDefinition
{
    std::string arg;
    std::string desc;
};

struct UserCommand
{
    std::string cmd;
    size_t arg = 0;
    bool executed = false;
};

struct Task
{
    std::string name;
    std::string command;
};

enum class ProcessState {
  kScheduled, kRunning, kComplete, kFailed
};

struct Process {
  bool destroy_entity_on_finish = false;
  std::string shell_command;
  std::unique_ptr<TinyProcessLib::Process> handle;
  ProcessState state = ProcessState::kScheduled;
};

enum class ProcessEventType {
    kStdout, kStderr, kExit
};

struct ProcessEvent
{
    entt::entity process;
    ProcessEventType type;
    std::string data;
};

enum class OutputMode {
  kSilent, kStream, kFailure
};

struct Output {
  OutputMode mode = OutputMode::kFailure;
  std::string stdout_line_buffer;
  std::string stderr_line_buffer;
  std::vector<std::string> lines;
  int lines_streamed = 0;
};

struct OutputSearch {
  int search_start;
  int search_end;
};

struct ConsoleOutput {
  std::vector<std::string> lines;
  std::vector<std::string> file_links;
  int lines_processed = 0;
  int lines_displayed = 0;
};

enum class DebugStatus {
    kNotRequired, kRequired, kAttached
};

struct Execution {
  std::string name;
  std::string shell_command;
  std::chrono::system_clock::time_point start_time;
  bool is_pinned = false;
  bool repeat_until_fail = false;
  int task_id;
  DebugStatus debug = DebugStatus::kNotRequired;
};

struct GtestTest {
  std::string name;
  std::string duration;
  int buffer_start;
  int buffer_end;
};

enum class GtestExecutionState {
    kRunning, kParsing, kParsed, kFinished
};

struct GtestExecution {
  bool display_progress = false;
  bool rerun_of_single_test = false;
  std::vector<GtestTest> tests;
  std::vector<int> failed_test_ids;
  int test_count = 0;
  int lines_parsed = 0;
  std::optional<int> current_test;
  std::string total_duration;
  GtestExecutionState state = GtestExecutionState::kRunning;
};

struct ToSchedule {};

class OsApi {
public:
  virtual void Init();
  virtual std::istream& In();
  virtual std::ostream& Out();
  virtual std::ostream& Err();
  virtual std::string GetEnv(const std::string& name);
  virtual void SetEnv(const std::string& name, const std::string& value);
  virtual void SetCurrentPath(const std::filesystem::path& path);
  virtual std::filesystem::path GetCurrentPath();
  virtual std::filesystem::path AbsolutePath(const std::filesystem::path& path);
  virtual bool ReadFile(const std::filesystem::path& path, std::string& data);
  virtual void WriteFile(const std::filesystem::path& path,
                         const std::string& data);
  virtual bool FileExists(const std::filesystem::path& path);
  virtual int Exec(const std::vector<const char*>& args);
  virtual void StartProcess(
      Process& process,
      const std::function<void(const char*, size_t)>& stdout_cb,
      const std::function<void(const char*, size_t)>& stderr_cb,
      const std::function<void()>& exit_cb);
  virtual void FinishProcess(Process& process);
  virtual int GetProcessExitCode(Process& process);
  virtual std::chrono::system_clock::time_point TimeNow();
protected:
  void StartProcessProtected(
      Process& process,
      const std::function<void(const char*, size_t)>& stdout_cb,
      const std::function<void(const char*, size_t)>& stderr_cb,
      const std::function<void()>& exit_cb);
};

struct Cdt {
  // Execution entities to execute where first entity
  // is the first execution to execute
  std::deque<entt::entity> execs_to_run;
  // History of executed entities where first entity
  // is the most recently executed entity
  std::deque<entt::entity> exec_history;
  entt::registry registry;
  moodycamel::BlockingConcurrentQueue<ProcessEvent> proc_event_queue;
  ConsoleOutput output;
  std::vector<std::string> kUsrCmdNames;
  std::vector<UserCommandDefinition> kUsrCmdDefs;
  UserCommand last_usr_cmd;
  std::vector<Task> tasks;
  std::vector<std::vector<size_t>> pre_tasks;
  std::string open_in_editor_cmd;
  std::string debug_cmd;
  const char* cdt_executable;
  std::filesystem::path user_config_path;
  std::filesystem::path tasks_config_path;
  std::optional<std::string> selected_config_profile;
  std::vector<std::string> config_errors;
  std::optional<entt::entity> selected_exec;
  OsApi* os;
};

bool InitCdt(int argc, const char** argv, Cdt& cdt);
bool WillWaitForInput(Cdt& cdt);
void ExecCdtSystems(Cdt& cdt);

#endif
