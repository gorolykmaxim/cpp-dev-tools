#include "find_in_files_controller.h"

#include "application.h"
#include "database.h"
#include "git_system.h"
#include "io_task.h"
#include "path.h"
#include "theme.h"
#include "threads.h"

#define LOG() qDebug() << "[FindInFilesController]"

using ResultBatch = QList<FileSearchResult>;

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
      search_results(new FileSearchResultListModel(this)),
      selected_file_cursor_position(-1),
      formatter(new SyntaxFormatter(this)) {
  Application& app = Application::Get();
  app.view.SetWindowTitle("Find In Files");
  connect(search_results, &TextListModel::selectedItemChanged, this,
          &FindInFilesController::OnSelectedResultChanged);
  connect(this, &FindInFilesController::optionsChanged, this,
          &FindInFilesController::SaveSearchTermAndOptions);
  connect(&search_result_watcher, &QFutureWatcher<ResultBatch>::resultReadyAt,
          this, &FindInFilesController::OnResultFound);
  connect(&search_result_watcher, &QFutureWatcher<ResultBatch>::finished, this,
          &FindInFilesController::OnSearchComplete);
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

FindInFilesController::~FindInFilesController() {
  search_result_watcher.cancel();
}

QString FindInFilesController::GetSearchStatus() const {
  QString result;
  result += QString::number(search_results->rowCount(QModelIndex())) +
            " results in " +
            QString::number(search_results->CountUniqueFiles()) + " files. ";
  result += search_result_watcher.isRunning() ? "Searching..." : "Complete.";
  return result;
}

static bool IsBinaryFile(const QString& line, bool& seen_replacement_char,
                         bool& seen_null) {
  static const QString kReplacementChar("\uFFFD");
  if (line.contains(kReplacementChar)) {
    seen_replacement_char = true;
  }
  if (line.contains('\0')) {
    seen_null = true;
  }
  return seen_replacement_char && seen_null;
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

static ResultBatch Find(const FindInFilesOptions& options,
                        const QString& search_term,
                        const QPromise<ResultBatch>& promise,
                        const QString& line, int column, int offset,
                        const QString& file_name) {
  ResultBatch results;
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
      result.line = column;
      result.col = row + 1;
      result.offset = offset + row;
      result.match_length = search_term.size();
      results.append(result);
    }
    pos = row + search_term.size() + 1;
    if (promise.isCanceled()) {
      break;
    }
  }
  return results;
}

static ResultBatch FindRegex(const QRegularExpression& search_term_regex,
                             const QPromise<ResultBatch>& promise,
                             const QString& line, int column, int offset,
                             const QString& file_name) {
  ResultBatch results;
  for (auto it = search_term_regex.globalMatch(line); it.hasNext();) {
    QRegularExpressionMatch match = it.next();
    FileSearchResult result;
    result.file_path = file_name;
    result.match =
        HighlightMatch(line, match.capturedStart(0), match.capturedLength(0));
    result.line = column;
    result.col = match.capturedStart(0) + 1;
    result.offset = offset + match.capturedStart(0);
    result.match_length = match.capturedLength(0);
    results.append(result);
    if (promise.isCanceled()) {
      break;
    }
  }
  return results;
}

void FindInFilesController::search() {
  LOG() << "Searching for" << search_term;
  selected_file_path.clear();
  selected_file_content.clear();
  selected_file_cursor_position = -1;
  emit selectedResultChanged();
  search_results->Clear();
  search_result_watcher.cancel();
  SaveSearchTermAndOptions();
  if (!search_term.isEmpty()) {
    QString folder = Application::Get().project.GetCurrentProject().path;
    QString search_term = this->search_term;
    FindInFilesOptions options = this->options;
    QFuture<ResultBatch> future = IoTask::Run<ResultBatch>(
        [options, search_term, folder](QPromise<ResultBatch>& promise) {
          QRegularExpression search_term_regex;
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
          QList<QString> folders = {folder};
          if (options.include_external_search_folders) {
            QList<QString> external = Database::ExecQueryAndRead<QString>(
                "SELECT * FROM external_search_folder",
                &Database::ReadStringFromSql);
            folders.append(external);
          }
          QList<QString> paths_to_exclude;
          if (options.exclude_git_ignored_files) {
            paths_to_exclude = GitSystem::FindIgnoredPathsSync();
          }
          QStringList files_to_scan;
          for (const QString& folder : folders) {
            QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
              files_to_scan.append(it.next());
            }
          }
          QList<std::pair<int, int>> ranges = Threads::SplitArrayAmongThreads(
              files_to_scan.size());
          QtConcurrent::blockingMap(ranges, [&files_to_scan, &paths_to_exclude,
                                             &options, &search_term_regex,
                                             &search_term, &promise](std::pair<int, int> range) {
            for (int i = range.first; i < range.second; i++) {
              QFile file(files_to_scan[i]);
              bool file_excluded = false;
              for (const QString& excluded_path : paths_to_exclude) {
                if (file.fileName().startsWith(excluded_path)) {
                  file_excluded = true;
                  break;
                }
              }
              bool not_included =
                  !options.files_to_include.isEmpty() &&
                  !Path::MatchesWildcard(file.fileName(),
                                         options.files_to_include);
              bool excluded = !options.files_to_exclude.isEmpty() &&
                              Path::MatchesWildcard(file.fileName(),
                                                    options.files_to_exclude);
              if (file_excluded || not_included || excluded ||
                  !file.open(QIODevice::ReadOnly)) {
                continue;
              }
              QTextStream stream(&file);
              ResultBatch file_results;
              bool seen_replacement_char = false;
              bool seen_null = false;
              int column = 1;
              int line_offset = 0;
              QString line;
              while (!stream.atEnd()) {
                stream.readLineInto(&line);
                if (IsBinaryFile(line, seen_replacement_char, seen_null)) {
                  LOG() << "Skipping binary file" << file.fileName();
                  file_results.clear();
                  break;
                }
                if (options.regexp) {
                  file_results.append(FindRegex(search_term_regex, promise,
                                                line, column, line_offset,
                                                file.fileName()));
                } else {
                  file_results.append(Find(options, search_term, promise, line,
                                           column, line_offset,
                                           file.fileName()));
                }
                if (promise.isCanceled()) {
                  LOG() << "Searching for" << search_term
                        << "has been cancelled";
                  return;
                }
                column++;
                line_offset += line.size() + 1;
              }
              if (!file_results.isEmpty()) {
                promise.addResult(file_results);
              }
            }
          });
        });
    search_result_watcher.setFuture(future);
  }
  emit searchStatusChanged();
}

void FindInFilesController::OnSelectedResultChanged() {
  int selected_result = search_results->GetSelectedItemIndex();
  if (selected_result < 0) {
    return;
  }
  LOG() << "Selected search result" << selected_result;
  const FileSearchResult& result = search_results->At(selected_result);
  formatter->DetectLanguageByFile(result.file_path);
  if (selected_file_path != result.file_path) {
    IoTask::Run<QString>(
        this,
        [result] {
          LOG() << "Reading contents of" << result.file_path;
          QFile file(result.file_path);
          if (!file.open(QIODeviceBase::ReadOnly)) {
            return "Failed to open file: " + file.errorString();
          }
          QString content(file.readAll());
          content.remove('\r');
          return content;
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
  Application::Get().editor.OpenFile(result.file_path, result.line,
                                     result.col);
}

void FindInFilesController::OnResultFound(int i) {
  ResultBatch results = search_result_watcher.resultAt(i);
  search_results->Append(results);
  emit searchStatusChanged();
}

void FindInFilesController::OnSearchComplete() {
  LOG() << "Search is complete";
  emit searchStatusChanged();
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

void FileSearchResultListModel::Append(const ResultBatch& items) {
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
