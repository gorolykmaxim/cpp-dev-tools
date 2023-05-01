#include "find_in_files_controller.h"

#include "application.h"
#include "database.h"
#include "git_system.h"
#include "language_keywords.h"
#include "path.h"
#include "theme.h"

#define LOG() qDebug() << "[FindInFilesController]"

FindInFilesController::FindInFilesController(QObject* parent)
    : QObject(parent),
      find_task(nullptr),
      search_results(new FileSearchResultListModel(this)),
      selected_result(-1),
      selected_file_cursor_position(-1),
      formatter(new FileSearchResultFormatter(this)) {}

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
  int old_result_line = -1;
  if (selected_result >= 0) {
    old_result_line = search_results->At(selected_result).column - 1;
  }
  selected_result = i;
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
    return Path::GetFileName(result.file_path);
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

static QTextCharFormat TextColorToFormat(const QString& hex) {
  QTextCharFormat format;
  format.setForeground(QBrush(QColor::fromString(hex)));
  return format;
}

static bool CompareLength(const QString& a, const QString& b) {
  return a.size() > b.size();
}

static QRegularExpression KeywordsRegExp(
    QStringList words, QRegularExpression::PatternOptions options =
                           QRegularExpression::NoPatternOption) {
  std::sort(words.begin(), words.end(), CompareLength);
  return QRegularExpression("\\b" + words.join("\\b|\\b") + "\\b", options);
}

static QRegularExpression KeywordsRegExpNoBoundaries(QStringList words) {
  std::sort(words.begin(), words.end(), CompareLength);
  return QRegularExpression(words.join('|'));
}

FileSearchResultFormatter::FileSearchResultFormatter(QObject* parent)
    : TextAreaFormatter(parent) {
  Theme theme;
  result_format.setBackground(
      QBrush(QColor::fromString(theme.kColorBgHighlight)));
  QTextCharFormat function_name_format = TextColorToFormat("#dcdcaa");
  QTextCharFormat comment_format = TextColorToFormat("#6a9956");
  QTextCharFormat language_keyword_format1 = TextColorToFormat("#569cd6");
  QTextCharFormat language_keyword_format2 = TextColorToFormat("#c586c0");
  TextFormat number_format{QRegularExpression("\\b([0-9.]+|0x[0-9.a-f]+)\\b"),
                           TextColorToFormat("#b5cea8")};
  TextFormat string_format{QRegularExpression("(\"|').*?(?<!\\\\)(\"|')"),
                           TextColorToFormat("#c98e75")};
  TextFormat slash_comment_format{QRegularExpression("\\/\\/.*$"),
                                  comment_format};
  cmake_formats = {
      TextFormat{QRegularExpression("\\b[a-zA-Z0-9\\_]+\\s?\\("),
                 function_name_format, -1},
      number_format,
      TextFormat{QRegularExpression("#.*$"), comment_format},
  };
  sql_formats = {
      TextFormat{KeywordsRegExp(kSqlKeywords,
                                QRegularExpression::CaseInsensitiveOption),
                 language_keyword_format1},
      TextFormat{KeywordsRegExp({"TRUE", "FALSE"},
                                QRegularExpression::CaseInsensitiveOption),
                 language_keyword_format2},
      number_format,
      string_format,
      TextFormat{QRegularExpression("--.*$"), comment_format},
  };
  qml_formats = {
      TextFormat{KeywordsRegExp(kQmlKeywords), language_keyword_format2},
      TextFormat{QRegularExpression("[a-zA-Z0-9.]+\\s{"), function_name_format,
                 -1},
      TextFormat{QRegularExpression("[a-zA-Z0-9.]+\\s?:"),
                 language_keyword_format1, -1},
      number_format,
      string_format,
      slash_comment_format,
  };
  cpp_formats = {
      TextFormat{KeywordsRegExp(kCppKeywords), language_keyword_format1},
      number_format,
      TextFormat{KeywordsRegExpNoBoundaries(kCPreprocessorKeywords),
                 language_keyword_format2},
      string_format,
      slash_comment_format,
  };
  js_formats = {
      TextFormat{KeywordsRegExp(kJsKeywords), language_keyword_format1},
      number_format,
      string_format,
      slash_comment_format,
  };
}

QList<TextSectionFormat> FileSearchResultFormatter::Format(
    const QString& text, const QTextBlock& block) {
  QList<TextSectionFormat> results;
  for (const TextFormat& format : current_language_formats) {
    for (auto it = format.regex.globalMatch(text); it.hasNext();) {
      QRegularExpressionMatch m = it.next();
      TextSectionFormat f;
      f.section.start = m.capturedStart(0);
      f.section.end = m.capturedEnd(0) - 1 + format.match_end_offset;
      f.format = format.format;
      results.append(f);
    }
  }
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
  if (result.file_path.endsWith("CMakeLists.txt") ||
      result.file_path.endsWith(".cmake")) {
    current_language_formats = cmake_formats;
  } else if (result.file_path.endsWith(".sql")) {
    current_language_formats = sql_formats;
  } else if (result.file_path.endsWith(".qml")) {
    current_language_formats = qml_formats;
  } else if (result.file_path.endsWith(".h") ||
             result.file_path.endsWith(".hxx") ||
             result.file_path.endsWith(".hpp") ||
             result.file_path.endsWith(".c") ||
             result.file_path.endsWith(".cc") ||
             result.file_path.endsWith(".cxx") ||
             result.file_path.endsWith(".cpp")) {
    current_language_formats = cpp_formats;
  } else if (result.file_path.endsWith(".js")) {
    current_language_formats = js_formats;
  } else {
    current_language_formats.clear();
  }
}
