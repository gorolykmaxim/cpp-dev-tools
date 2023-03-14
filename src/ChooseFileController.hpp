#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "QVariantListModel.hpp"

class ChooseFileController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString path READ GetPath WRITE SetPath NOTIFY pathChanged)
  Q_PROPERTY(SimpleQVariantListModel* suggestions MEMBER suggestions CONSTANT)
 public:
  explicit ChooseFileController(QObject* parent = nullptr);
  void SetPath(const QString& path);
  QString GetPath() const;

 public slots:
  void PickSuggestion(int i);
  void OpenOrCreateFile();
  void CreateFile();

 signals:
  void pathChanged();
  void fileChosen(const QString& path);

 private:
  void SortAndFilterSuggestions();
  QString GetResultPath() const;

  QString folder, file;
  SimpleQVariantListModel* suggestions;
};
