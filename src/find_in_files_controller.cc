#include "find_in_files_controller.h"

#include "application.h"

#define LOG() qDebug() << "[FindInFilesController]"

FindInFilesController::FindInFilesController(QObject* parent)
    : QObject(parent),
      find_task(nullptr),
      search_results(new FileSearchResultListModel(this)),
      selected_result(-1) {}

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
  QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    QFile file(it.next());
    if (!file.open(QIODevice::ReadOnly)) {
      continue;
    }
    QTextStream stream(&file);
    QList<FileSearchResult> file_results;
    bool seen_replacement_char = false;
    bool seen_null = false;
    int column = 1;
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
        file_results.append(FindRegex(line, column, file.fileName()));
      } else {
        file_results.append(Find(line, column, file.fileName()));
      }
      if (is_cancelled) {
        LOG() << "Searching for" << search_term << "has been cancelled";
        return;
      }
      column++;
    }
    if (!file_results.isEmpty()) {
      emit resultsFound(file_results);
    }
  }
}

QList<FileSearchResult> FindInFilesTask::Find(const QString& line, int column,
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
    const QString& line, int column, const QString& file_name) const {
  QList<FileSearchResult> results;
  for (auto it = search_term_regex.globalMatch(line); it.hasNext();) {
    QRegularExpressionMatch match = it.next();
    FileSearchResult result;
    result.file_path = file_name;
    result.match =
        HighlightMatch(line, match.capturedStart(0), match.capturedLength(0));
    result.column = column;
    result.row = match.capturedStart(0) + 1;
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
         regexp == another.regexp;
}

bool FindInFilesOptions::operator!=(const FindInFilesOptions& another) const {
  return !(*this == another);
}
