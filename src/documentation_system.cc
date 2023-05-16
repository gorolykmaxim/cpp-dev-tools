#include "documentation_system.h"

#include <QDesktopServices>
#include <QtGlobal>

#include "application.h"
#include "database.h"
#include "io_task.h"
#include "path.h"

#define LOG() qDebug() << "[DocumentationSystem]"

DocumentListModel::DocumentListModel(QObject* parent) : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "subTitle"}, {2, "icon"}});
  searchable_roles = {0, 1};
  SetEmptyListPlaceholder(Placeholder("No documentation found"));
}

QVariantList DocumentListModel::GetRow(int i) const {
  const Document& doc = list[i];
  return {doc.file_name, doc.path, "help_outline"};
}

int DocumentListModel::GetRowCount() const { return list.size(); }

DocumentationSystem::DocumentationSystem()
    : QObject(nullptr), documents(new DocumentListModel(this)) {}

struct DocumentationSearch {
  QSet<QString> documentation_folders;
  QList<Document> documents;
};

void DocumentationSystem::displayDocumentation() {
  Application::Get().view.SetWindowTitle("Documentation");
  documents->SetPlaceholder(Placeholder("Looking for documentation..."));
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
          for (const QString& folder : qAsConst(result.documentation_folders)) {
            LOG() << "Searching for documentation in" << folder;
            QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
              Document doc;
              doc.path = it.next();
              if (!doc.path.endsWith(".html") && !doc.path.endsWith(".htm")) {
                continue;
              }
              doc.file_name = Path::GetFileName(doc.path);
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
        documents->SetPlaceholder(Placeholder());
      });
}

void DocumentationSystem::openSelectedDocument() {
  int i = documents->GetSelectedItemIndex();
  if (i < 0) {
    return;
  }
  const Document& doc = documents->list[i];
  QUrl url("file:///" + doc.path);
  LOG() << "Opening document" << url;
  QDesktopServices::openUrl(url);
}
