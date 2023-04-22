#include "find_in_files_controller.h"

#include "application.h"

#define LOG() qDebug() << "[FindInFilesController]"

FindInFilesController::FindInFilesController(QObject* parent)
    : QObject(parent), search_results(new FileSearchResultListModel(this)) {}

QString FindInFilesController::GetSearchStatus() const {
  if (!find_task) {
    return "";
  }
  QString result;
  result += QString::number(search_results->rowCount(QModelIndex())) +
            " results in " +
            QString::number(search_results->CountUniqueFiles()) + " files. ";
  result += find_task->IsFinished() ? "Complete." : "Searching...";
  return result;
}

void FindInFilesController::search() {
  LOG() << "Searching for" << search_term;
  search_results->Clear();
  emit searchStatusChanged();
  if (search_term.isEmpty()) {
    return;
  }
  if (find_task) {
    find_task->deleteLater();
  }
  find_task = new FindInFilesTask(this, search_term);
  QObject::connect(find_task, &FindInFilesTask::finished, this,
                   &FindInFilesController::OnSearchComplete);
  QObject::connect(find_task, &FindInFilesTask::resultsFound, this,
                   &FindInFilesController::OnResultFound);
  find_task->Run();
}

void FindInFilesController::OnResultFound(QList<FileSearchResult> results) {
  search_results->Append(results);
  emit searchStatusChanged();
}

void FindInFilesController::OnSearchComplete() {
  LOG() << "Search is complete";
  emit searchStatusChanged();
}

FindInFilesTask::FindInFilesTask(QObject* parent, const QString& search_term)
    : BaseIoTask(parent), search_term(search_term) {
  folder = Application::Get().project.GetCurrentProject().path;
}

static QString HighlighMatch(const QString& line, int match_pos,
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
    int column = 0;
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
      int pos = 0;
      while (true) {
        int row = line.indexOf(search_term, pos);
        if (row < 0) {
          break;
        }
        FileSearchResult result;
        result.file_path = file.fileName();
        result.match = HighlighMatch(line, row, search_term.size());
        result.column = column;
        result.row = row;
        file_results.append(result);
        pos = row + search_term.size() + 1;
      }
      column++;
    }
    if (!file_results.isEmpty()) {
      emit resultsFound(file_results);
    }
  }
}

FileSearchResultListModel::FileSearchResultListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int FileSearchResultListModel::rowCount(const QModelIndex&) const {
  return list.size();
}

QHash<int, QByteArray> FileSearchResultListModel::roleNames() const {
  return {{0, "title"}, {1, "subTitle"}};
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
