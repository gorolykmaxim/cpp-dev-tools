#ifndef CDT_H
#define CDT_H

#include <chrono>
#include <cstddef>
#include <functional>
#include <istream>
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

struct UserCommandDefinition
{
    std::string cmd;
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

enum class ExecutionEventType {
    kStdout, kStderr, kExit
};

struct ExecutionEvent
{
    Entity exec;
    ExecutionEventType type;
    std::string data;
};

enum class ExecutionState {
    kRunning, kComplete, kFailed
};

struct Execution
{
    std::string name;
    std::string shell_command;
    ExecutionState state = ExecutionState::kRunning;
    std::string stdout_line_buffer;
    std::string stderr_line_buffer;
    std::chrono::system_clock::time_point start_time;
    bool is_pinned = false;
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

enum class TextBufferType {
    kProcess, kGtest, kOutput
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
    virtual void KillProcess(Entity e, std::unordered_map<Entity, std::unique_ptr<TinyProcessLib::Process>>& processes);
    virtual void ExecProcess(const std::string& shell_cmd);
    virtual void StartProcess(Entity e, const std::string& shell_cmd,
                               const std::function<void(const char*, size_t)>& stdout_cb,
                               const std::function<void(const char*, size_t)>& stderr_cb,
                               const std::function<void()>& exit_cb,
                               std::unordered_map<Entity, std::unique_ptr<TinyProcessLib::Process>>& processes);
    virtual int GetProcessExitCode(Entity e, std::unordered_map<Entity, std::unique_ptr<TinyProcessLib::Process>>& processes);
    virtual std::chrono::system_clock::time_point TimeNow();
};

struct Cdt {
    std::deque<Entity> execs_to_run_in_order; // Execution entities to execute where first entity is the first execution to execute
    std::deque<Entity> exec_history; // History of executed entities where first entity is the most recently executed entity
    std::unordered_map<Entity, size_t> task_ids;
    std::unordered_map<Entity, std::unique_ptr<TinyProcessLib::Process>> processes;
    std::unordered_map<Entity, Execution> execs;
    std::unordered_map<Entity, ExecutionOutput> exec_outputs;
    std::unordered_map<Entity, GtestExecution> gtest_execs;
    std::unordered_map<TextBufferType, std::unordered_map<Entity, std::vector<std::string>>> text_buffers;
    std::unordered_map<Entity, TextBufferSearch> text_buffer_searchs;
    std::unordered_set<Entity> repeat_until_fail;
    std::unordered_set<Entity> stream_output;
    std::unordered_set<Entity> debug;
    moodycamel::BlockingConcurrentQueue<ExecutionEvent> exec_event_queue;
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
