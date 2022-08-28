#include "process/config.hpp"
#include "common.hpp"
#include <QFile>
#include <QIODevice>
#include <QJsonParseError>
#include <QStandardPaths>

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

WriteJsonFile::WriteJsonFile(const QString& path, const QJsonDocument& json)
    : path(path), json(json) {
  EXEC_NEXT(Write);
}

void WriteJsonFile::Write(Application& app) {
  QPtr<WriteJsonFile> self = app.runtime.SharedPtr(this);
  app.threads.ScheduleIO([self] () {
    QFile file(self->path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      self->error = "Failed to open json file '" + file.fileName() +
                    "': " + file.errorString();
      return;
    }
    file.write(self->json.toJson());
  }, [self, &app] () {app.runtime.WakeUpAndExecute(*self);});
  EXEC_NEXT(Process::kNoopExecute);
}

Initialize::Initialize() {
  EXEC_NEXT(ReadConfig);
}

void Initialize::ReadConfig(Application& app) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QString user_config_path = home + "/.cpp-dev-tools.json";
  read_config = app.runtime.Schedule<ReadJsonFile>(this, user_config_path);
  EXEC_NEXT(LoadConfig);
}

void Initialize::LoadConfig(Application& app) {
  app.user_config.LoadFrom(read_config->json);
  app.runtime.Schedule<WriteJsonFile>(this, read_config->path,
                                      app.user_config.Save());
}
