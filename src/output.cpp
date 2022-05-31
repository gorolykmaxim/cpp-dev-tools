#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
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
  for (auto [_, output, proc]: cdt.registry.view<Output, Process>().each()) {
    if (output.mode == OutputMode::kStream ||
        proc.state == ProcessState::kFailed &&
        output.mode == OutputMode::kFailure) {
      int new_lines_count = output.lines.size() - output.lines_streamed;
      cdt.output.lines.reserve(cdt.output.lines.size() + new_lines_count);
      cdt.output.lines.insert(cdt.output.lines.end(),
                              output.lines.begin() + output.lines_streamed,
                              output.lines.end());
      output.lines_streamed += new_lines_count;
    }
  }
}

std::string FindNextSubmatch(const std::smatch& match,
                             int& current_submatch) {
  for (; current_submatch < match.size(); current_submatch++) {
    if (match[current_submatch].matched) {
      return match[current_submatch++].str();
    }
  }
  return "";
}

void FindAndHighlightFileLinks(Cdt& cdt) {
  static const std::regex kFileLinkRegex(
      "(\\/[^:]+):([0-9]+):?([0-9]+)?"
#ifdef _WIN32
      "|([A-Z]\\:\\\\[^:]+)\\(([0-9]+),?([0-9]+)?\\)"
      "|([A-Z]\\:\\\\[^:]+):([0-9]+):([0-9]+)?"
#endif
  );
  if (cdt.open_in_editor_cmd.empty()) {
    return;
  }
  for (int i = cdt.output.lines_processed; i < cdt.output.lines.size(); i++) {
    std::string& line = cdt.output.lines[i];
    std::stringstream highlighted_line;
    int last_link_end_pos = 0;
    for (std::sregex_iterator it(line.begin(), line.end(), kFileLinkRegex);
         it != std::sregex_iterator();
         it++) {
      std::stringstream link_stream;
      int current_submatch = 1;
      link_stream << FindNextSubmatch(*it, current_submatch) << ':'
                  << FindNextSubmatch(*it, current_submatch);
      std::string column_num = FindNextSubmatch(*it, current_submatch);
      if (!column_num.empty()) {
        link_stream << ':' << column_num;
      }
      std::string link = link_stream.str();
      cdt.output.file_links.push_back(link);
      int chars_before_link = it->position() - last_link_end_pos;
      highlighted_line << line.substr(last_link_end_pos, chars_before_link)
                       << kTcMagenta << '[' << kOpen
                       << cdt.output.file_links.size() << "] " << link
                       << kTcReset;
      last_link_end_pos = it->position() + it->length();
    }
    int chars_after_link = line.size() - last_link_end_pos;
    highlighted_line << line.substr(last_link_end_pos, chars_after_link);
    line = highlighted_line.str();
  }
  cdt.output.lines_processed = cdt.output.lines.size();
}

void PrintExecutionOutput(Cdt& cdt) {
  for (int i = cdt.output.lines_displayed; i < cdt.output.lines.size(); i++) {
    cdt.os->Out() << cdt.output.lines[i] << std::endl;
  }
  cdt.output.lines_displayed = cdt.output.lines.size();
}

void OpenFileLink(Cdt& cdt) {
  if (!AcceptUsrCmd(kOpen, cdt.last_usr_cmd)) return;
  if (cdt.open_in_editor_cmd.empty()) {
    WarnUserConfigPropNotSpecified(kOpenInEditorCommandProperty, cdt);
  } else if (cdt.output.file_links.empty()) {
    cdt.os->Out() << kTcGreen << "No file links in the output" << kTcReset
                  << std::endl;
  } else {
    if (IsCmdArgInRange(cdt.last_usr_cmd, cdt.output.file_links)) {
      int link_index = cdt.last_usr_cmd.arg - 1;
      std::string& link = cdt.output.file_links[link_index];
      std::unordered_map<std::string, std::string> vars = {{"", link}};
      std::string shell_command = FormatTemplate(cdt.open_in_editor_cmd, vars);
      entt::entity entity = cdt.registry.create();
      cdt.registry.emplace<Process>(entity, true, shell_command);
      cdt.registry.emplace<Output>(entity);
    } else {
      cdt.os->Out() << kTcGreen << "Last execution output:" << kTcReset
                    << std::endl;
      for (std::string& line: cdt.output.lines) {
        cdt.os->Out() << line << std::endl;
      }
    }
  }
}

void SearchThroughLastExecutionOutput(Cdt& cdt) {
  if (!AcceptUsrCmd(kSearch, cdt.last_usr_cmd)) return;
  if (cdt.exec_history.empty()) {
    cdt.os->Out() << kTcGreen << "No task has been executed yet" << kTcReset
                  << std::endl;
  } else {
    entt::entity entity = cdt.selected_exec ?
                          *cdt.selected_exec :
                          cdt.exec_history.front();
    Output& output = cdt.registry.get<Output>(entity);
    cdt.registry.emplace<OutputSearch>(entity, 0,
                                       static_cast<int>(output.lines.size()));
  }
}

void SearchThroughOutput(Cdt& cdt) {
  std::vector<entt::entity> to_delete;
  auto view = cdt.registry.view<OutputSearch, Output>();
  for (auto [entity, search, output]: view.each()) {
    std::string input = ReadInputFromStdin("Regular expression: ", cdt);
    try {
      std::regex regex(input);
      bool results_found = false;
      for (int i = search.search_start; i < search.search_end; i++) {
        const std::string& line = output.lines[i];
        std::unordered_set<size_t> highlight_starts, highlight_ends;
        for (std::sregex_iterator it(line.begin(), line.end(), regex);
             it != std::sregex_iterator(); it++) {
          results_found = true;
          highlight_starts.insert(it->position());
          highlight_ends.insert(it->position() + it->length() - 1);
        }
        if (highlight_starts.empty()) {
          continue;
        }
        cdt.os->Out() << kTcMagenta << i - search.search_start + 1 << ':'
                      << kTcReset;
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
        cdt.os->Out() << kTcGreen << "No matches found" << kTcReset
                      << std::endl;
      }
    } catch (const std::regex_error& e) {
      cdt.os->Out() << kTcRed << "Invalid regular expression '" << input
                    << "': " << e.what() << kTcReset << std::endl;
    }
    to_delete.push_back(entity);
  }
  cdt.registry.erase<OutputSearch>(to_delete.begin(), to_delete.end());
}
