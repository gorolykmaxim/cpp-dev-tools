#include "ReadJsonFile.hpp"
#include "Common.hpp"
#include <QFile>
#include <QIODevice>
#include <QJsonParseError>

ReadJsonFile::ReadJsonFile(const QString& path) : path(path) {
  EXEC_NEXT(Read);
}

void ReadJsonFile::Read(Application& app) {
  QPtr<ReadJsonFile> self = app.runtime.SharedPtr(this);
  app.threads.ScheduleIO([self] () {
    QFile file(self->path);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
      self->error = "Failed to open json file '" + file.fileName() +
                    "': " + file.errorString();
      return;
    }
    QString data = file.readAll();
    QJsonParseError json_error;
    self->json = QJsonDocument::fromJson(data.toUtf8(), &json_error);
    if (self->json.isNull()) {
      self->error = "Failed to parse json file '" + file.fileName() +
                    "': " + json_error.errorString();
    }
  }, [self, &app] () {app.runtime.WakeUpAndExecute(*self);});
  EXEC_NEXT(Process::kNoopExecute);
}
