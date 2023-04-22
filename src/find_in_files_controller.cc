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
  QSet<QString> files;
  for (const FileSearchResult& result : search_results->list) {
    files.insert(result.file_path);
  }
  result += QString::number(search_results->list.size()) + " results in " +
            QString::number(files.size()) + " files. ";
  result += find_task->IsFinished() ? "Complete." : "Searching...";
  return result;
}

void FindInFilesController::search() {
  LOG() << "Searching for" << search_term;
  search_results->list.clear();
  emit searchStatusChanged();
  search_results->Load();
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
  for (const FileSearchResult& result : results) {
    search_results->list.append(result);
  }
  emit searchStatusChanged();
  search_results->Load();
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
    : QVariantListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "subTitle"}});
}

QVariantList FileSearchResultListModel::GetRow(int i) const {
  const FileSearchResult& result = list[i];
  int j = result.file_path.lastIndexOf('/');
  QString file_name = j < 0 ? result.file_path : result.file_path.sliced(j + 1);
  return {result.match, file_name};
}

int FileSearchResultListModel::GetRowCount() const { return list.size(); }
