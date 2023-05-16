#ifndef DOCUMENTATIONSYSTEM_H
#define DOCUMENTATIONSYSTEM_H

#include <QObject>

#include "text_list_model.h"

struct Document {
  QString file_name;
  QString path;
};

class DocumentListModel : public TextListModel {
 public:
  explicit DocumentListModel(QObject* parent);

  QList<Document> list;

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;
};

class DocumentationSystem : public QObject {
  Q_OBJECT
  Q_PROPERTY(DocumentListModel* documents MEMBER documents CONSTANT)
 public:
  DocumentationSystem();
 public slots:
  void displayDocumentation();
  void openSelectedDocument();

 private:
  DocumentListModel* documents;
  QSet<QString> documentation_folders;
};

#endif  // DOCUMENTATIONSYSTEM_H
