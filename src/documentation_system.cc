#include "documentation_system.h"

#include <QDesktopServices>

#include "application.h"
#include "database.h"
#include "io_task.h"

DocumentListModel::DocumentListModel(QObject* parent)
    : QVariantListModel(parent) {
  SetRoleNames({{0, "idx"}, {1, "title"}, {2, "subTitle"}});
  searchable_roles = {1, 2};
}

QVariantList DocumentListModel::GetRow(int i) const {
  const Document& doc = list[i];
  return {i, doc.relative_file_path, doc.documentation_folder};
}

int DocumentListModel::GetRowCount() const { return list.size(); }

DocumentationSystem::DocumentationSystem()
    : QObject(nullptr),
      is_loading(false),
      documents(new DocumentListModel(this)) {}

bool DocumentationSystem::ShouldShowPlaceholder() const {
  return is_loading || documents->list.isEmpty();
}

bool DocumentationSystem::IsLoading() const { return is_loading; }

struct DocumentationSearch {
  QSet<QString> documentation_folders;
  QList<Document> documents;
};

void DocumentationSystem::displayDocumentation() {
  Application::Get().view.SetWindowTitle("Documentation");
  SetIsLoading(true);
  QSet<QString> old_folders = documentation_folders;
  IoTask::Run<DocumentationSearch>(
      this,
      [old_folders] {
        DocumentationSearch result;
        QStringList folders = Database::ExecQueryAndRead<QString>(
            "SELECT * FROM documentation_folder", &Database::ReadStringFromSql);
        result.documentation_folders =
            QSet<QString>(folders.begin(), folders.end());
        if (old_folders != result.documentation_folders) {
          for (const QString& folder : result.documentation_folders) {
            QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
              QString file_path = it.next();
              Document doc;
              doc.documentation_folder = folder;
              doc.relative_file_path = file_path.remove(folder);
              result.documents.append(doc);
            }
          }
        }
        return result;
      },
      [this](DocumentationSearch result) {
        if (documentation_folders != result.documentation_folders) {
          documentation_folders = result.documentation_folders;
          documents->list = result.documents;
          documents->Load();
        }
        SetIsLoading(false);
      });
}

void DocumentationSystem::openDocument(int i) {
  const Document& doc = documents->list[i];
  QUrl url("file:///" + doc.documentation_folder + doc.relative_file_path);
  QDesktopServices::openUrl(url);
}

void DocumentationSystem::SetIsLoading(bool value) {
  is_loading = value;
  emit isLoadingChanged();
}
