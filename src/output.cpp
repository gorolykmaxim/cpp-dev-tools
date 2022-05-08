#include <regex>
#include <sstream>

#include "output.h"
#include "common.h"

static std::string kOpen;
static std::string kSearch;

void InitOutput(Cdt& cdt) {
    kOpen = DefineUserCommand("o", {"ind", "Open the file link with the specified index in your code editor"}, cdt);
    kSearch = DefineUserCommand("s",{ "", "Search through output of the selected executed task with the specified regular expression"}, cdt);
}

void StreamExecutionOutput(Cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        if (Find(proc.first, cdt.stream_output)) {
            MoveTextBuffer(proc.first, TextBufferType::kProcess, TextBufferType::kOutput, cdt.text_buffers);
        }
    }
}

void FindAndHighlightFileLinks(Cdt& cdt) {
    static const std::regex kFileLinkRegex("(\\/[^:]+):([0-9]+):?([0-9]+)?");
    if (cdt.open_in_editor_cmd.str.empty()) return;
    for (auto& out: cdt.exec_outputs) {
        auto& buffer = cdt.text_buffers[out.first][TextBufferType::kOutput];
        for (auto i = out.second.lines_processed; i < buffer.size(); i++) {
            auto& line = buffer[i];
            std::stringstream highlighted_line;
            auto last_link_end_pos = 0;
            for (auto it = std::sregex_iterator(line.begin(), line.end(), kFileLinkRegex); it != std::sregex_iterator(); it++) {
                std::stringstream link_stream;
                link_stream << (*it)[1] << ':' << (*it)[2];
                if (it->size() > 3 && (*it)[3].matched) {
                    link_stream << ':' << (*it)[3];
                }
                const auto link = link_stream.str();
                out.second.file_links.push_back(link);
                highlighted_line << line.substr(last_link_end_pos, it->position() - last_link_end_pos) << kTcMagenta << '[' << kOpen << out.second.file_links.size() << "] " << link << kTcReset;
                last_link_end_pos = it->position() + it->length();
            }
            highlighted_line << line.substr(last_link_end_pos, line.size() - last_link_end_pos);
            line = highlighted_line.str();
        }
    }
}

void PrintExecutionOutput(Cdt& cdt) {
    for (auto& out: cdt.exec_outputs) {
        auto& buffer = cdt.text_buffers[out.first][TextBufferType::kOutput];
        for (auto i = out.second.lines_processed; i < buffer.size(); i++) {
            cdt.os->Out() << buffer[i] << std::endl;
        }
        out.second.lines_processed = buffer.size();
    }
}

void StreamExecutionOutputOnFailure(Cdt& cdt) {
    for (const auto& proc: cdt.processes) {
        if (cdt.execs[proc.first].state == ExecutionState::kFailed && !Find(proc.first, cdt.stream_output)) {
            MoveTextBuffer(proc.first, TextBufferType::kProcess, TextBufferType::kOutput, cdt.text_buffers);
        }
    }
}

void OpenFileLink(Cdt& cdt) {
    if (!AcceptUsrCmd(kOpen, cdt.last_usr_cmd)) return;
    ExecutionOutput* exec_output = nullptr;
    const std::vector<std::string>* buffer = nullptr;
    if (!cdt.exec_history.empty()) {
        Entity entity = cdt.selected_exec ? *cdt.selected_exec : cdt.exec_history.front();
        exec_output = &cdt.exec_outputs[entity];
        buffer = &cdt.text_buffers[entity][TextBufferType::kOutput];
    }
    if (cdt.open_in_editor_cmd.str.empty()) {
        WarnUserConfigPropNotSpecified(kOpenInEditorCommandProperty, cdt);
    } else if (!exec_output || exec_output->file_links.empty()) {
        cdt.os->Out() << kTcGreen << "No file links in the output" << kTcReset << std::endl;
    } else if (exec_output) {
        if (IsCmdArgInRange(cdt.last_usr_cmd, exec_output->file_links)) {
            const auto& link = exec_output->file_links[cdt.last_usr_cmd.arg - 1];
            const auto& shell_command = FormatTemplate(cdt.open_in_editor_cmd, link);
            cdt.os->ExecProcess(shell_command);
        } else {
            cdt.os->Out() << kTcGreen << "Last execution output:" << kTcReset << std::endl;
            for (const auto& line: *buffer) {
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
        Entity entity = cdt.selected_exec ? *cdt.selected_exec : cdt.exec_history.front();
        size_t buffer_size = cdt.text_buffers[entity][TextBufferType::kOutput].size();
        cdt.text_buffer_searchs[entity] = TextBufferSearch{TextBufferType::kOutput, 0, buffer_size};
    }
}

void SearchThroughTextBuffer(Cdt& cdt) {
    std::vector<Entity> to_destroy;
    for (const auto& it: cdt.text_buffer_searchs) {
        const auto input = ReadInputFromStdin("Regular expression: ", cdt);
        const auto& search = it.second;
        const auto& buffer = cdt.text_buffers[it.first][search.type];
        try {
            std::regex regex(input);
            bool results_found = false;
            for (auto i = search.search_start; i < search.search_end; i++) {
                const auto& line = buffer[i];
                const auto start = std::sregex_iterator(line.begin(), line.end(), regex);
                const auto end = std::sregex_iterator();
                std::unordered_set<size_t> highlight_starts, highlight_ends;
                for (auto it = start; it != end; it++) {
                    results_found = true;
                    highlight_starts.insert(it->position());
                    highlight_ends.insert(it->position() + it->length() - 1);
                }
                if (highlight_starts.empty()) {
                    continue;
                }
                cdt.os->Out() << kTcMagenta << i - search.search_start + 1 << ':' << kTcReset;
                for (auto j = 0; j < line.size(); j++) {
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
        to_destroy.push_back(it.first);
    }
    DestroyComponents(to_destroy, cdt.text_buffer_searchs);
}
