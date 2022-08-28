#include "WriteJsonFile.hpp"
#include "Common.hpp"
#include <QFile>
#include <QIODevice>

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
