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
  Q_PROPERTY(
      bool showPlaceholder READ ShouldShowPlaceholder NOTIFY isLoadingChanged)
  Q_PROPERTY(bool isLoading READ IsLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(DocumentListModel* documents MEMBER documents CONSTANT)
 public:
  DocumentationSystem();
  bool ShouldShowPlaceholder() const;
  bool IsLoading() const;
 public slots:
  void displayDocumentation();
  void openDocument(int i);
 signals:
  void isLoadingChanged();

 private:
  void SetIsLoading(bool value);

  bool is_loading;
  DocumentListModel* documents;
  QSet<QString> documentation_folders;
};

#endif  // DOCUMENTATIONSYSTEM_H
