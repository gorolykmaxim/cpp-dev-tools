#include "find_in_files_controller.h"

#include "application.h"
#include "database.h"
#include "git_system.h"
#include "path.h"
#include "theme.h"

#define LOG() qDebug() << "[FindInFilesController]"

static std::pair<QString, FindInFilesOptions> ReadFromSql(QSqlQuery& sql) {
  QString search_term = sql.value(1).toString();
  FindInFilesOptions options;
  options.match_case = sql.value(2).toBool();
  options.match_whole_word = sql.value(3).toBool();
  options.regexp = sql.value(4).toBool();
  options.include_external_search_folders = sql.value(5).toBool();
  options.exclude_git_ignored_files = sql.value(6).toBool();
  options.expanded = sql.value(7).toBool();
  options.files_to_include = sql.value(8).toString();
  options.files_to_exclude = sql.value(9).toString();
  return std::make_pair(search_term, options);
}

FindInFilesController::FindInFilesController(QObject* parent)
    : QObject(parent),
      find_task(nullptr),
      search_results(new FileSearchResultListModel(this)),
      selected_file_cursor_position(-1),
      formatter(new FileSearchResultFormatter(this)) {
  Application& app = Application::Get();
  app.view.SetWindowTitle("Find In Files");
  QObject::connect(search_results, &TextListModel::selectedItemChanged, this,
                   &FindInFilesController::OnSelectedResultChanged);
  QObject::connect(this, &FindInFilesController::optionsChanged, this,
                   &FindInFilesController::SaveSearchTermAndOptions);
  QUuid project_id = app.project.GetCurrentProject().id;
  using Result = std::pair<QString, FindInFilesOptions>;
  IoTask::Run<QList<Result>>(
      this,
      [project_id] {
        return Database::ExecQueryAndRead<Result>(
            "SELECT * FROM find_in_files_context WHERE project_id=?",
            ReadFromSql, {project_id});
      },
      [this](QList<Result> results) {
        if (results.isEmpty()) {
          return;
        }
        search_term = results[0].first;
        options = results[0].second;
        emit searchTermChanged();
        emit optionsChanged();
      });
}

FindInFilesController::~FindInFilesController() { CancelSearchIfRunning(); }

QString FindInFilesController::GetSearchStatus() const {
  QString result;
  result += QString::number(search_results->rowCount(QModelIndex())) +
            " results in " +
            QString::number(search_results->CountUniqueFiles()) + " files. ";
  result += !find_task ? "Complete." : "Searching...";
  return result;
}

void FindInFilesController::search() {
  LOG() << "Searching for" << search_term;
  prev_selected_result = -1;
  selected_file_path.clear();
  selected_file_content.clear();
  selected_file_cursor_position = -1;
  emit selectedResultChanged();
  search_results->Clear();
  CancelSearchIfRunning();
  emit searchStatusChanged();
  SaveSearchTermAndOptions();
  if (search_term.isEmpty()) {
    return;
  }
  find_task = new FindInFilesTask(search_term, options);
  QObject::connect(find_task, &FindInFilesTask::finished, this,
                   &FindInFilesController::OnSearchComplete);
  QObject::connect(find_task, &FindInFilesTask::resultsFound, this,
                   &FindInFilesController::OnResultFound);
  find_task->Run();
}

void FindInFilesController::OnSelectedResultChanged() {
  int selected_result = search_results->GetSelectedItemIndex();
  if (selected_result < 0) {
    return;
  }
  LOG() << "Selected search result" << selected_result;
  int old_result_line = -1;
  if (prev_selected_result >= 0) {
    old_result_line = search_results->At(prev_selected_result).column - 1;
  }
  prev_selected_result = selected_result;
  const FileSearchResult& result = search_results->At(selected_result);
  int new_result_line = result.column - 1;
  formatter->SetResult(result);
  if (selected_file_path != result.file_path) {
    IoTask::Run<QString>(
        this,
        [result] {
          LOG() << "Reading contents of" << result.file_path;
          QFile file(result.file_path);
          if (!file.open(QIODeviceBase::ReadOnly)) {
            return "Failed to open file: " + file.errorString();
          }
          return QString(file.readAll());
        },
        [this, result](QString content) {
          selected_file_path = result.file_path;
          selected_file_content = content;
          selected_file_cursor_position = result.offset;
          emit selectedResultChanged();
        });
  } else {
    selected_file_cursor_position = result.offset;
    emit selectedResultChanged();
    if (old_result_line >= 0) {
      emit rehighlightBlockByLineNumber(old_result_line);
    }
    emit rehighlightBlockByLineNumber(new_result_line);
  }
}

void FindInFilesController::SaveSearchTermAndOptions() {
  QUuid project_id = Application::Get().project.GetCurrentProject().id;
  Database::ExecCmdAsync(
      "INSERT OR REPLACE INTO find_in_files_context "
      "VALUES(?, ?, ?, ?, ?, ?, "
      "?, ?, ?, ?)",
      {project_id, search_term, options.match_case, options.match_whole_word,
       options.regexp, options.include_external_search_folders,
       options.exclude_git_ignored_files, options.expanded,
       options.files_to_include, options.files_to_exclude});
}

void FindInFilesController::openSelectedResultInEditor() {
  int selected_result = search_results->GetSelectedItemIndex();
  if (selected_result < 0) {
    return;
  }
  const FileSearchResult& result = search_results->At(selected_result);
  Application::Get().editor.OpenFile(result.file_path, result.column,
                                     result.row);
}

void FindInFilesController::OnResultFound(QList<FileSearchResult> results) {
  search_results->Append(results);
  emit searchStatusChanged();
}

void FindInFilesController::OnSearchComplete() {
  LOG() << "Search is complete";
  find_task = nullptr;
  emit searchStatusChanged();
}

void FindInFilesController::CancelSearchIfRunning() {
  if (find_task) {
    find_task->is_cancelled = true;
  }
}

static bool MatchesWildcard(const QString& str, const QString& pattern) {
  QStringList parts = pattern.split('*');
  int pos = 0;
  for (int i = 0; i < parts.size(); i++) {
    const QString& part = parts[i];
    if (part.isEmpty()) {
      continue;
    }
    int j = str.indexOf(part, pos, Qt::CaseSensitive);
    if (j < 0) {
      return false;
    }
    if (i == 0 && j != 0) {
      return false;
    } else if (i == parts.size() - 1 && part.size() + j != str.size()) {
      return false;
    }
    pos = part.size() + j + 1;
  }
  return true;
}

FindInFilesTask::FindInFilesTask(const QString& search_term,
                                 FindInFilesOptions options)
    : BaseIoTask(), search_term(search_term), options(options) {
  folder = Application::Get().project.GetCurrentProject().path;
  if (options.regexp) {
    QRegularExpression::PatternOptions regex_opts =
        QRegularExpression::NoPatternOption;
    if (!options.match_case) {
      regex_opts |= QRegularExpression::CaseInsensitiveOption;
    }
    QString pattern = search_term;
    if (options.match_whole_word) {
      pattern = "\\b" + pattern + "\\b";
    }
    search_term_regex = QRegularExpression(pattern, regex_opts);
  }
}

static QString HighlightMatch(const QString& line, int match_pos,
                              int match_length) {
  const static int kMaxLength = 200;
  QString before = line.sliced(0, match_pos).toHtmlEscaped();
  QString match = line.sliced(match_pos, match_length).toHtmlEscaped();
  QString after = line.sliced(match_pos + match_length,
                              line.size() - match_pos - match_length)
                      .toHtmlEscaped();
  int padding = match_length < kMaxLength ? (kMaxLength - match.size()) / 2 : 0;
  if (before.size() > padding) {
    before = "..." + before.sliced(before.size() - padding);
  }
  if (after.size() > padding) {
    after = after.sliced(0, padding) + "...";
  }
  return before + "<b>" + match + "</b>" + after;
}

void FindInFilesTask::RunInBackground() {
  QList<QString> folders = {folder};
  if (options.include_external_search_folders) {
    QList<QString> external = Database::ExecQueryAndRead<QString>(
        "SELECT * FROM external_search_folder", &Database::ReadStringFromSql);
    folders.append(external);
  }
  QList<QString> paths_to_exclude;
  if (options.exclude_git_ignored_files) {
    paths_to_exclude = GitSystem::FindIgnoredPathsSync();
  }
  for (const QString& folder : folders) {
    QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
      QFile file(it.next());
      bool file_excluded = false;
      for (const QString& excluded_path : paths_to_exclude) {
        if (file.fileName().startsWith(excluded_path)) {
          file_excluded = true;
          break;
        }
      }
      bool not_included =
          !options.files_to_include.isEmpty() &&
          !MatchesWildcard(file.fileName(), options.files_to_include);
      bool excluded =
          !options.files_to_exclude.isEmpty() &&
          MatchesWildcard(file.fileName(), options.files_to_exclude);
      if (file_excluded || not_included || excluded ||
          !file.open(QIODevice::ReadOnly)) {
        continue;
      }
      QTextStream stream(&file);
      QList<FileSearchResult> file_results;
      bool seen_replacement_char = false;
      bool seen_null = false;
      int column = 1;
      int line_offset = 0;
      while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.contains("\uFFFD")) {
          seen_replacement_char = true;
        }
        if (line.contains('\0')) {
          seen_null = true;
        }
        if (seen_replacement_char && seen_null) {
          LOG() << "Skipping binary file" << file.fileName();
          file_results.clear();
          break;
        }
        if (options.regexp) {
          file_results.append(
              FindRegex(line, column, line_offset, file.fileName()));
        } else {
          file_results.append(Find(line, column, line_offset, file.fileName()));
        }
        if (is_cancelled) {
          LOG() << "Searching for" << search_term << "has been cancelled";
          return;
        }
        column++;
        line_offset += line.size() + 1;
      }
      if (!file_results.isEmpty()) {
        emit resultsFound(file_results);
      }
    }
  }
}

QList<FileSearchResult> FindInFilesTask::Find(const QString& line, int column,
                                              int offset,
                                              const QString& file_name) const {
  QList<FileSearchResult> results;
  int pos = 0;
  while (true) {
    Qt::CaseSensitivity sensitivity;
    if (options.match_case) {
      sensitivity = Qt::CaseSensitive;
    } else {
      sensitivity = Qt::CaseInsensitive;
    }
    int row = line.indexOf(search_term, pos, sensitivity);
    if (row < 0) {
      break;
    }
    bool letter_before = row > 0 && line[row - 1].isLetter();
    bool letter_after = row + search_term.size() < line.size() &&
                        line[row + search_term.size()].isLetter();
    if (!options.match_whole_word || (!letter_before && !letter_after)) {
      FileSearchResult result;
      result.file_path = file_name;
      result.match = HighlightMatch(line, row, search_term.size());
      result.column = column;
      result.row = row + 1;
      result.offset = offset + row;
      result.match_length = search_term.size();
      results.append(result);
    }
    pos = row + search_term.size() + 1;
    if (is_cancelled) {
      break;
    }
  }
  return results;
}

QList<FileSearchResult> FindInFilesTask::FindRegex(
    const QString& line, int column, int offset,
    const QString& file_name) const {
  QList<FileSearchResult> results;
  for (auto it = search_term_regex.globalMatch(line); it.hasNext();) {
    QRegularExpressionMatch match = it.next();
    FileSearchResult result;
    result.file_path = file_name;
    result.match =
        HighlightMatch(line, match.capturedStart(0), match.capturedLength(0));
    result.column = column;
    result.row = match.capturedStart(0) + 1;
    result.offset = offset + match.capturedStart(0);
    result.match_length = match.capturedLength(0);
    results.append(result);
    if (is_cancelled) {
      break;
    }
  }
  return results;
}

FileSearchResultListModel::FileSearchResultListModel(QObject* parent)
    : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "subTitle"}});
}

void FileSearchResultListModel::Clear() {
  if (list.isEmpty()) {
    return;
  }
  int count = list.size();
  list.clear();
  unique_file_paths.clear();
  LoadRemoved(count);
}

void FileSearchResultListModel::Append(const QList<FileSearchResult>& items) {
  if (items.isEmpty()) {
    return;
  }
  int first = list.size();
  list.append(items);
  for (const FileSearchResult& result : items) {
    unique_file_paths.insert(result.file_path);
  }
  LoadNew(first, -1);
}

int FileSearchResultListModel::CountUniqueFiles() const {
  return unique_file_paths.size();
}

const FileSearchResult& FileSearchResultListModel::At(int i) const {
  return list[i];
}

QVariantList FileSearchResultListModel::GetRow(int i) const {
  const FileSearchResult& result = list[i];
  return {result.match, Path::GetFileName(result.file_path)};
}

int FileSearchResultListModel::GetRowCount() const { return list.size(); }

bool FindInFilesOptions::operator==(const FindInFilesOptions& another) const {
  return match_case == another.match_case &&
         match_whole_word == another.match_whole_word &&
         regexp == another.regexp &&
         include_external_search_folders ==
             another.include_external_search_folders &&
         files_to_include == another.files_to_include &&
         files_to_exclude == another.files_to_exclude &&
         exclude_git_ignored_files == another.exclude_git_ignored_files &&
         expanded == another.expanded;
}

bool FindInFilesOptions::operator!=(const FindInFilesOptions& another) const {
  return !(*this == another);
}

FileSearchResultFormatter::FileSearchResultFormatter(QObject* parent)
    : TextAreaFormatter(parent), syntax_formatter(new SyntaxFormatter(this)) {
  Theme theme;
  result_format.setBackground(
      QBrush(QColor::fromString(theme.kColorBgHighlight)));
}

QList<TextSectionFormat> FileSearchResultFormatter::Format(
    const QString& text, const QTextBlock& block) {
  QList<TextSectionFormat> results;
  results.append(syntax_formatter->Format(text, block));
  if (block.firstLineNumber() == result.column - 1) {
    TextSectionFormat f;
    f.section.start = result.row - 1;
    f.section.end = f.section.start + result.match_length - 1;
    f.format = result_format;
    results.append(f);
  }
  return results;
}

void FileSearchResultFormatter::SetResult(const FileSearchResult& result) {
  this->result = result;
  syntax_formatter->DetectLanguageByFile(result.file_path);
}
