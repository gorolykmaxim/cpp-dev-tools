#include "JsonFileProcess.hpp"
#include <QFile>
#include <QDebug>
#include <QJsonParseError>
#include <utility>

JsonFileProcess::JsonFileProcess(JsonOperation operation, const QString& path,
                                 QJsonDocument json)
    : operation(operation), path(path), json(std::move(json)) {
  EXEC_NEXT(Run);
}


void JsonFileProcess::Run(Application& app) {
  QPtr<JsonFileProcess> self = app.runtime.SharedPtr(this);
  app.threads.ScheduleIO([self] () {
    QFile file(self->path);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
      self->error = "Failed to open JSON file '" + file.fileName() +
                    "': " + file.errorString();
      return;
    }
    if (self->operation == JsonOperation::kRead) {
      qDebug() << "Reading from JSON file" << self->path;
      QJsonParseError json_error;
      self->json = QJsonDocument::fromJson(file.readAll(), &json_error);
      if (self->json.isNull()) {
        self->error = "Failed to parse JSON file '" + file.fileName() +
                      "': " + json_error.errorString();
      }
    } else if (self->operation == JsonOperation::kWrite) {
      qDebug() << "Writing to JSON file" << self->path;
      file.write(self->json.toJson());
    }
  }, [self, &app] () {app.runtime.WakeUpAndExecute(*self);});
  EXEC_NEXT(Process::kNoopExecute);
}
