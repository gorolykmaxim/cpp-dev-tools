#include "find_in_files_controller.h"

#include "application.h"
#include "database.h"
#include "git_system.h"
#include "theme.h"

#define LOG() qDebug() << "[FindInFilesController]"

FindInFilesController::FindInFilesController(QObject* parent)
    : QObject(parent),
      find_task(nullptr),
      search_results(new FileSearchResultListModel(this)),
      selected_result(-1),
      selected_file_cursor_position(-1),
      formatter(new FileSearchResultFormatter(this, selected_result,
                                              search_results)) {}

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
  selected_result = -1;
  selected_file_path.clear();
  selected_file_content.clear();
  selected_file_cursor_position = -1;
  emit selectedResultChanged();
  search_results->Clear();
  CancelSearchIfRunning();
  emit searchStatusChanged();
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

void FindInFilesController::selectResult(int i) {
  LOG() << "Selected search result" << i;
  selected_result = i;
  const FileSearchResult& result = search_results->At(selected_result);
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
    emit rehighlight();
  }
}

void FindInFilesController::openSelectedResultInEditor() {
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
    : QAbstractListModel(parent) {}

int FileSearchResultListModel::rowCount(const QModelIndex&) const {
  return list.size();
}

QHash<int, QByteArray> FileSearchResultListModel::roleNames() const {
  return {{0, "idx"}, {1, "title"}, {2, "subTitle"}};
}

QVariant FileSearchResultListModel::data(const QModelIndex& index,
                                         int role) const {
  if (!index.isValid()) {
    return QVariant();
  }
  if (index.row() < 0 || index.row() >= list.size()) {
    return QVariant();
  }
  Q_ASSERT(roleNames().contains(role));
  const FileSearchResult& result = list[index.row()];
  if (role == 0) {
    return index.row();
  } else if (role == 1) {
    return result.match;
  } else {
    int i = result.file_path.lastIndexOf('/');
    return i < 0 ? result.file_path : result.file_path.sliced(i + 1);
  }
}

void FileSearchResultListModel::Clear() {
  if (list.isEmpty()) {
    return;
  }
  beginRemoveRows(QModelIndex(), 0, list.size() - 1);
  list.clear();
  unique_file_paths.clear();
  endRemoveRows();
}

void FileSearchResultListModel::Append(const QList<FileSearchResult>& items) {
  if (items.isEmpty()) {
    return;
  }
  beginInsertRows(QModelIndex(), list.size(), list.size() + items.size() - 1);
  list.append(items);
  for (const FileSearchResult& result : items) {
    unique_file_paths.insert(result.file_path);
  }
  endInsertRows();
}

int FileSearchResultListModel::CountUniqueFiles() const {
  return unique_file_paths.size();
}

const FileSearchResult& FileSearchResultListModel::At(int i) const {
  return list[i];
}

bool FindInFilesOptions::operator==(const FindInFilesOptions& another) const {
  return match_case == another.match_case &&
         match_whole_word == another.match_whole_word &&
         regexp == another.regexp &&
         include_external_search_folders ==
             another.include_external_search_folders &&
         files_to_include == another.files_to_include &&
         files_to_exclude == another.files_to_exclude &&
         exclude_git_ignored_files == another.exclude_git_ignored_files;
}

bool FindInFilesOptions::operator!=(const FindInFilesOptions& another) const {
  return !(*this == another);
}

FileSearchResultFormatter::FileSearchResultFormatter(
    QObject* parent, int& selected_result,
    FileSearchResultListModel* search_results)
    : TextAreaFormatter(parent),
      selected_result(selected_result),
      search_results(search_results) {
  Theme theme;
  QColor color = QColor::fromString(theme.kColorBgHighlight);
  result_format.setBackground(QBrush(color));
}

QList<TextSectionFormat> FileSearchResultFormatter::Format(
    const QString&, const QTextBlock& block) {
  QList<TextSectionFormat> results;
  if (selected_result < 0) {
    return results;
  }
  const FileSearchResult& result = search_results->At(selected_result);
  if (block.firstLineNumber() == result.column - 1) {
    TextSectionFormat f;
    f.section.start = result.row - 1;
    f.section.end = f.section.start + result.match_length - 1;
    f.format = result_format;
    results.append(f);
  }
  return results;
}
