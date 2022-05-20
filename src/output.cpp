#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "cdt.h"
#include "output.h"
#include "common.h"

static std::string kOpen;
static std::string kSearch;

void InitOutput(Cdt& cdt) {
    kOpen = DefineUserCommand("o", {"ind", "Open the file link with the specified index in your code editor"}, cdt);
    kSearch = DefineUserCommand("s",{ "", "Search through output of the selected executed task with the specified regular expression"}, cdt);
}

void StreamExecutionOutput(Cdt& cdt) {
    for (auto [_, proc, exec, buffer]: cdt.registry.view<Process, Execution, TextBuffer>().each()) {
        if (proc.stream_output || exec.state == ExecutionState::kFailed) {
            MoveTextBuffer(kBufferProcess, kBufferOutput, buffer);
        }
    }
}

void FindAndHighlightFileLinks(Cdt& cdt) {
    static const std::regex kFileLinkRegex("(\\/[^:]+):([0-9]+):?([0-9]+)?");
    if (cdt.open_in_editor_cmd.empty()) return;
    for (auto [_, output, text_buffers]: cdt.registry.view<ExecutionOutput, TextBuffer>().each()) {
        std::vector<std::string>& buffer = text_buffers.buffers[kBufferOutput];
        for (int i = output.lines_processed; i < buffer.size(); i++) {
            std::string& line = buffer[i];
            std::stringstream highlighted_line;
            int last_link_end_pos = 0;
            for (std::sregex_iterator it(line.begin(), line.end(), kFileLinkRegex); it != std::sregex_iterator(); it++) {
                std::stringstream link_stream;
                link_stream << (*it)[1] << ':' << (*it)[2];
                if (it->size() > 3 && (*it)[3].matched) {
                    link_stream << ':' << (*it)[3];
                }
                std::string link = link_stream.str();
                output.file_links.push_back(link);
                highlighted_line << line.substr(last_link_end_pos, it->position() - last_link_end_pos) << kTcMagenta << '[' << kOpen << output.file_links.size() << "] " << link << kTcReset;
                last_link_end_pos = it->position() + it->length();
            }
            highlighted_line << line.substr(last_link_end_pos, line.size() - last_link_end_pos);
            line = highlighted_line.str();
        }
    }
}

void PrintExecutionOutput(Cdt& cdt) {
    for (auto [_, output, text_buffers]: cdt.registry.view<ExecutionOutput, TextBuffer>().each()) {
        const std::vector<std::string>& buffer = text_buffers.buffers[kBufferOutput];
        for (int i = output.lines_processed; i < buffer.size(); i++) {
            cdt.os->Out() << buffer[i] << std::endl;
        }
        output.lines_processed = buffer.size();
    }
}

void OpenFileLink(Cdt& cdt) {
    if (!AcceptUsrCmd(kOpen, cdt.last_usr_cmd)) return;
    ExecutionOutput* exec_output = nullptr;
    std::vector<std::string>* buffer = nullptr;
    if (!cdt.exec_history.empty()) {
        entt::entity entity = cdt.selected_exec ?
                              *cdt.selected_exec :
                              cdt.exec_history.front();
        exec_output = cdt.registry.try_get<ExecutionOutput>(entity);
        buffer = &cdt.registry.get<TextBuffer>(entity).buffers[kBufferOutput];
    }
    if (cdt.open_in_editor_cmd.empty()) {
        WarnUserConfigPropNotSpecified(kOpenInEditorCommandProperty, cdt);
    } else if (!exec_output || exec_output->file_links.empty()) {
        cdt.os->Out() << kTcGreen << "No file links in the output" << kTcReset
                      << std::endl;
    } else if (exec_output) {
        if (IsCmdArgInRange(cdt.last_usr_cmd, exec_output->file_links)) {
            int link_index = cdt.last_usr_cmd.arg - 1;
            std::string& link = exec_output->file_links[link_index];
            std::string shell_command = FormatTemplate(cdt.open_in_editor_cmd,
                                                       "{}", link);
            cdt.os->ExecProcess(shell_command);
        } else {
            cdt.os->Out() << kTcGreen << "Last execution output:" << kTcReset
                          << std::endl;
            for (std::string& line: *buffer) {
                cdt.os->Out() << line << std::endl;
            }
        }
    }
}

void SearchThroughLastExecutionOutput(Cdt& cdt) {
    if (!AcceptUsrCmd(kSearch, cdt.last_usr_cmd)) return;
    if (cdt.exec_history.empty()) {
        cdt.os->Out() << kTcGreen << "No task has been executed yet" << kTcReset << std::endl;
    } else {
        entt::entity entity = cdt.selected_exec ? *cdt.selected_exec : cdt.exec_history.front();
        size_t buffer_size = cdt.registry.get<TextBuffer>(entity).buffers[kBufferOutput].size();
        cdt.registry.emplace<TextBufferSearch>(entity, kBufferOutput, size_t(0), buffer_size);
    }
}

void SearchThroughTextBuffer(Cdt& cdt) {
    std::vector<entt::entity> to_destroy;
    for (auto [entity, search, text_buffers]: cdt.registry.view<TextBufferSearch, TextBuffer>().each()) {
        std::string input = ReadInputFromStdin("Regular expression: ", cdt);
        const std::vector<std::string>& buffer = text_buffers.buffers[search.type];
        try {
            std::regex regex(input);
            bool results_found = false;
            for (int i = search.search_start; i < search.search_end; i++) {
                const std::string& line = buffer[i];
                std::unordered_set<size_t> highlight_starts, highlight_ends;
                for (std::sregex_iterator it(line.begin(), line.end(), regex); it != std::sregex_iterator(); it++) {
                    results_found = true;
                    highlight_starts.insert(it->position());
                    highlight_ends.insert(it->position() + it->length() - 1);
                }
                if (highlight_starts.empty()) {
                    continue;
                }
                cdt.os->Out() << kTcMagenta << i - search.search_start + 1 << ':' << kTcReset;
                for (int j = 0; j < line.size(); j++) {
                    if (highlight_starts.count(j) > 0) {
                        cdt.os->Out() << kTcGreen;
                    }
                    cdt.os->Out() << line[j];
                    if (highlight_ends.count(j) > 0) {
                        cdt.os->Out() << kTcReset;
                    }
                }
                cdt.os->Out() << std::endl;
            }
            if (!results_found) {
                cdt.os->Out() << kTcGreen << "No matches found" << kTcReset << std::endl;
            }
        } catch (const std::regex_error& e) {
            cdt.os->Out() << kTcRed << "Invalid regular expression '" << input << "': " << e.what() << kTcReset << std::endl;
        }
        to_destroy.push_back(entity);
    }
    cdt.registry.erase<TextBufferSearch>(to_destroy.begin(), to_destroy.end());
}
