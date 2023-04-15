#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "qvariant_list_model.h"

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
  void pickSuggestion(int i);
  void openOrCreateFile();

 signals:
  void pathChanged();
  void fileChosen(const QString& path);

 private:
  QString GetResultPath() const;
  void CreateFile();

  QString folder, file;
  SimpleQVariantListModel* suggestions;
};
