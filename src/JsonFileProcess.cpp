#include "JsonFileProcess.hpp"
#include "Threads.hpp"
#include "Process.hpp"

JsonFileProcess::JsonFileProcess(JsonOperation operation, const QString& path,
                                 QJsonDocument json)
    : operation(operation), path(path), json(std::move(json)) {
  EXEC_NEXT(Run);
}


void JsonFileProcess::Run(AppData& app) {
  QPtr<JsonFileProcess> self = ProcessSharedPtr(app, this);
  ScheduleIOTask(app, [self] () {
    QString parent_folder = QDir::cleanPath(self->path + "/..");
    QDir().mkpath(parent_folder);
    QFile file(self->path);
    QIODevice::OpenMode mode = QIODevice::ReadWrite | QIODevice::Text;
    if (self->operation == JsonOperation::kWrite) {
      mode |= QIODevice::Truncate;
    }
    if (!file.open(mode)) {
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
  }, [self, &app] () {WakeUpAndExecuteProcess(app, *self);});
  EXEC_NEXT(Noop);
}
