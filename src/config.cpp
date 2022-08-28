#include "config.hpp"
#include "common.hpp"
#include <QIODevice>
#include <QJsonParseError>
#include <QStandardPaths>

ReadJsonFile::ReadJsonFile(const QString& path) : file(path) {
  EXEC_NEXT(Read);
}

void ReadJsonFile::Read(Application& app) {
  QPtr<ReadJsonFile> self = app.runtime.SharedPtr(this);
  app.threads.ScheduleIO([self] () {
    if (!self->file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      self->error = "Failed to open json file '" + self->file.fileName() +
                    "': " + self->file.errorString();
      return;
    }
    QString data = self->file.readAll();
    QJsonParseError json_error;
    self->json = QJsonDocument::fromJson(data.toUtf8(), &json_error);
    if (self->json.isNull()) {
      self->error = "Failed to parse json file '" + self->file.fileName() +
                    "': " + json_error.errorString();
    }
  }, [self, &app] () {app.runtime.WakeUpAndExecute(*self);});
  EXEC_NEXT(Process::kNoopExecute);
}

Initialize::Initialize() {
  EXEC_NEXT(ReadConfigs);
}

void Initialize::ReadConfigs(Application& app) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QString user_config_path = home + "/.cpp-dev-tools.json";
  read_user_config = app.runtime.Schedule<ReadJsonFile>(this, user_config_path);
  EXEC_NEXT(DisplayConfigs);
}

void Initialize::DisplayConfigs(Application& app) {
  qDebug() << read_user_config->json.toJson() << read_user_config->error;
}
