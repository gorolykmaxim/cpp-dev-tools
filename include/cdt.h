#ifndef CDT_H
#define CDT_H

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

struct user_command_definition
{
    std::string cmd;
    std::string arg;
    std::string desc;
};

struct user_command
{
    std::string cmd;
    size_t arg = 0;
    bool executed = false;
};

struct task
{
    std::string name;
    std::string command;
};

typedef uint32_t entity;

enum execution_event_type {
    execution_event_type_stdout, execution_event_type_stderr, execution_event_type_exit
};

struct execution_event
{
    entity exec;
    execution_event_type type;
    std::string data;
};

enum execution_state {
    execution_state_running, execution_state_complete, execution_state_failed
};

struct execution
{
    std::string name;
    std::string shell_command;
    execution_state state = execution_state_running;
    std::string stdout_line_buffer;
    std::string stderr_line_buffer;
};

struct gtest_test {
    std::string name;
    std::string duration;
    size_t buffer_start;
    size_t buffer_end;
};

enum gtest_execution_state {
    gtest_execution_state_running, gtest_execution_state_parsing, gtest_execution_state_parsed, gtest_execution_state_finished
};

struct gtest_execution {
    bool rerun_of_single_test = false;
    std::vector<gtest_test> tests;
    std::vector<size_t> failed_test_ids;
    size_t test_count = 0;
    std::optional<size_t> current_test;
    std::string total_duration;
    gtest_execution_state state = gtest_execution_state_running;
};

struct execution_output
{
    size_t lines_processed = 0;
    std::vector<std::string> file_links;
};

struct template_string
{
    std::string str;
    size_t arg_pos;
};

enum text_buffer_type {
    text_buffer_type_process, text_buffer_type_gtest, text_buffer_type_output
};

struct text_buffer_search {
    text_buffer_type type;
    size_t search_start;
    size_t search_end;
};

class os_api {
public:
    virtual std::istream& in();
    virtual std::ostream& out();
    virtual std::ostream& err();
    virtual std::string get_env(const std::string& name);
    virtual void set_env(const std::string& name, const std::string& value);
    virtual void set_current_path(const std::filesystem::path& path);
    virtual std::filesystem::path get_current_path();
    virtual bool read_file(const std::filesystem::path& path, std::string& data);
    virtual void write_file(const std::filesystem::path& path, const std::string& data);
    virtual bool file_exists(const std::filesystem::path& path);
    virtual void signal(int signal, void(*handler)(int));
    virtual void raise_signal(int signal);
    virtual int exec(const std::vector<const char*>& args);
    virtual void kill_process(entity e, std::unordered_map<entity, std::unique_ptr<TinyProcessLib::Process>>& processes);
    virtual void exec_process(const std::string& shell_cmd);
    virtual void start_process(entity e, const std::string& shell_cmd,
                               const std::function<void(const char*, size_t)>& stdout_cb,
                               const std::function<void(const char*, size_t)>& stderr_cb,
                               const std::function<void()>& exit_cb,
                               std::unordered_map<entity, std::unique_ptr<TinyProcessLib::Process>>& processes);
    virtual int get_process_exit_code(entity e, std::unordered_map<entity, std::unique_ptr<TinyProcessLib::Process>>& processes);
};

struct cdt {
    std::deque<entity> execs_to_run_in_order;
    std::unordered_map<entity, size_t> task_ids;
    std::unordered_map<entity, std::unique_ptr<TinyProcessLib::Process>> processes;
    std::unordered_map<entity, execution> execs;
    std::unordered_map<entity, execution_output> exec_outputs;
    std::unordered_map<entity, gtest_execution> gtest_execs;
    std::unordered_map<text_buffer_type, std::unordered_map<entity, std::vector<std::string>>> text_buffers;
    std::unordered_map<entity, text_buffer_search> text_buffer_searchs;
    std::unordered_set<entity> repeat_until_fail;
    std::unordered_set<entity> stream_output;
    std::unordered_set<entity> debug;
    moodycamel::BlockingConcurrentQueue<execution_event> exec_event_queue;
    user_command last_usr_cmd;
    std::vector<task> tasks;
    std::vector<std::vector<size_t>> pre_tasks;
    template_string open_in_editor_cmd;
    template_string debug_cmd;
    template_string execute_in_new_terminal_tab_cmd;
    const char* cdt_executable;
    std::filesystem::path user_config_path;
    std::filesystem::path tasks_config_path;
    std::vector<std::string> config_errors;
    entity entity_seed = 1;
    entity last_entity;
    os_api* os;
};

bool init_cdt(int argc, const char** argv, cdt& cdt);
bool will_wait_for_input(cdt& cdt);
void exec_cdt_systems(cdt& cdt);

#endif
