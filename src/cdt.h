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

const std::string kTcRed = "\033[31m";
const std::string kTcGreen = "\033[32m";
const std::string kTcBlue = "\033[34m";
const std::string kTcMagenta = "\033[35m";
const std::string kTcReset = "\033[0m";

const std::string kOpenInEditorCommandProperty = "open_in_editor_command";
const std::string kExecuteInNewTerminalTabCommandProperty = "execute_in_new_terminal_tab_command";
const std::string kDebugCommandProperty = "debug_command";
const std::string kEnvVarLastCommand = "LAST_COMMAND";
const std::string kTemplateArgPlaceholder = "{}";
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

typedef uint32_t Entity;

enum class DebugStatus {
    kNotRequired, kRequired, kAttached
};

struct Process {
    std::string shell_command;
    std::string stdout_line_buffer;
    std::string stderr_line_buffer;
    std::unique_ptr<TinyProcessLib::Process> handle;
    bool stream_output = false;
    DebugStatus debug = DebugStatus::kNotRequired;
};

enum class ProcessEventType {
    kStdout, kStderr, kExit
};

struct ProcessEvent
{
    Entity process;
    ProcessEventType type;
    std::string data;
};

enum class ExecutionState {
    kRunning, kComplete, kFailed
};

struct Execution
{
    std::string name;
    ExecutionState state = ExecutionState::kRunning;
    std::chrono::system_clock::time_point start_time;
    bool is_pinned = false;
    bool repeat_until_fail = false;
    size_t task_id;
};

struct GtestTest {
    std::string name;
    std::string duration;
    size_t buffer_start;
    size_t buffer_end;
};

enum class GtestExecutionState {
    kRunning, kParsing, kParsed, kFinished
};

struct GtestExecution {
    bool rerun_of_single_test = false;
    std::vector<GtestTest> tests;
    std::vector<size_t> failed_test_ids;
    size_t test_count = 0;
    std::optional<size_t> current_test;
    std::string total_duration;
    GtestExecutionState state = GtestExecutionState::kRunning;
};

struct ExecutionOutput
{
    size_t lines_processed = 0;
    std::vector<std::string> file_links;
};

struct TemplateString
{
    std::string str;
    size_t arg_pos;
};

enum TextBufferType {
    kBufferProcess = 0,
    kBufferGtest,
    kBufferOutput
};

struct TextBuffer {
    std::vector<std::vector<std::string>> buffers = std::vector<std::vector<std::string>>(kBufferOutput + 1);
};

struct TextBufferSearch {
    TextBufferType type;
    size_t search_start;
    size_t search_end;
};

class OsApi {
public:
    virtual std::istream& In();
    virtual std::ostream& Out();
    virtual std::ostream& Err();
    virtual std::string GetEnv(const std::string& name);
    virtual void SetEnv(const std::string& name, const std::string& value);
    virtual void SetCurrentPath(const std::filesystem::path& path);
    virtual std::filesystem::path GetCurrentPath();
    virtual bool ReadFile(const std::filesystem::path& path, std::string& data);
    virtual void WriteFile(const std::filesystem::path& path, const std::string& data);
    virtual bool FileExists(const std::filesystem::path& path);
    virtual void Signal(int signal, void(*handler)(int));
    virtual void RaiseSignal(int signal);
    virtual int Exec(const std::vector<const char*>& args);
    virtual void KillProcess(Process& process);
    virtual void ExecProcess(const std::string& shell_cmd);
    virtual void StartProcess(Process& process,
                              const std::function<void(const char*, size_t)>& stdout_cb,
                              const std::function<void(const char*, size_t)>& stderr_cb,
                              const std::function<void()>& exit_cb);
    virtual int GetProcessExitCode(Process& process);
    virtual std::chrono::system_clock::time_point TimeNow();
};

struct Cdt {
    std::deque<Entity> execs_to_schedule;
    std::deque<Entity> execs_to_run; // Execution entities to execute where first entity is the first execution to execute
    std::deque<Entity> running_execs;
    std::deque<Entity> exec_history; // History of executed entities where first entity is the most recently executed entity
    std::unordered_map<Entity, Process> processes;
    std::unordered_map<Entity, Execution> execs;
    std::unordered_map<Entity, ExecutionOutput> exec_outputs;
    std::unordered_map<Entity, GtestExecution> gtest_execs;
    std::unordered_map<Entity, TextBuffer> text_buffers;
    std::unordered_map<Entity, TextBufferSearch> text_buffer_searchs;
    moodycamel::BlockingConcurrentQueue<ProcessEvent> proc_event_queue;
    std::vector<std::string> kUsrCmdNames;
    std::vector<UserCommandDefinition> kUsrCmdDefs;
    UserCommand last_usr_cmd;
    std::vector<Task> tasks;
    std::vector<std::vector<size_t>> pre_tasks;
    TemplateString open_in_editor_cmd;
    TemplateString debug_cmd;
    TemplateString execute_in_new_terminal_tab_cmd;
    const char* cdt_executable;
    std::filesystem::path user_config_path;
    std::filesystem::path tasks_config_path;
    std::vector<std::string> config_errors;
    Entity entity_seed = 1;
    std::optional<Entity> selected_exec;
    OsApi* os;
};

bool InitCdt(int argc, const char** argv, Cdt& cdt);
bool WillWaitForInput(Cdt& cdt);
void ExecCdtSystems(Cdt& cdt);

#endif
