#include "blockingconcurrentqueue.h"
#include "concurrentqueue.h"
#include "process.hpp"
#include <cstddef>
#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

static std::function<void(const char*, size_t)> write_to(moodycamel::BlockingConcurrentQueue<std::string>& output, std::string& line_buffer) {
    return [&output, &line_buffer] (const char* data, size_t size) {
        const auto to_process = line_buffer + std::string(data, size);
        std::stringstream line;
        for (const auto c: to_process) {
            if (c == '\n') {
                output.enqueue(line.str());
                line = std::stringstream();
            } else {
                line << c;
            }
        }
    };
}

struct cdt_test: public testing::Test {
protected:
    moodycamel::BlockingConcurrentQueue<std::string> output;
    std::string stdout_buffer, stderr_buffer;
    std::unique_ptr<TinyProcessLib::Process> proc;

    virtual void SetUp() override {
        std::filesystem::path cwd(BINARY_DIR);
        std::vector<std::string> args = {cwd / BINARY_NAME, cwd / "test-tasks.json"};
        proc = std::make_unique<TinyProcessLib::Process>(args, "", write_to(output, stdout_buffer), write_to(output, stderr_buffer), true);
    }
    virtual void TearDown() override {
        proc->kill();
    }
    std::string get_out_line() {
        std::string msg;
        output.wait_dequeue(msg);
        return msg;
    }
};

TEST_F(cdt_test, start_and_view_tasks) {
    ASSERT_EQ("Type \x1B[32mh\x1B[0m to see list of all the user commands.", get_out_line());
    ASSERT_EQ("\x1B[32mTasks:\x1B[0m", get_out_line());
    ASSERT_EQ("1 \"hello world\"", get_out_line());
    ASSERT_EQ(0, output.size_approx());
}