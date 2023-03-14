#pragma once

#include <QObject>
#include <QtQmlIntegration>

class RunTaskController : public QObject {
  Q_OBJECT
  QML_ELEMENT
 public:
  explicit RunTaskController(QObject* parent = nullptr);
 public slots:
  void ExecuteTask(const QString& command) const;
};
