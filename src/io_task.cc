#include "io_task.h"

#include "application.h"

BaseIoTask::BaseIoTask(QObject *parent) : QObject(parent) {}

void BaseIoTask::Run() {
  (void)QtConcurrent::run(&Application::Get().io_thread_pool, [this] {
    RunInBackground();
    emit finished();
  });
}

class VoidIoTask : public BaseIoTask {
 public:
  explicit VoidIoTask(const std::function<void()> &on_background,
                      QObject *parent);

 protected:
  void RunInBackground();

 private:
  std::function<void()> on_background;
};

void IoTask::Run(QObject *requestor, const std::function<void()> &on_io_thread,
                 const std::function<void()> &on_ui_thread) {
  VoidIoTask *task = new VoidIoTask(on_io_thread, requestor);
  QObject::connect(task, &BaseIoTask::finished, requestor,
                   [task, on_ui_thread] {
                     on_ui_thread();
                     task->deleteLater();
                   });
  task->Run();
}

VoidIoTask::VoidIoTask(const std::function<void()> &on_background,
                       QObject *parent)
    : BaseIoTask(parent), on_background(on_background) {}

void VoidIoTask::RunInBackground() { on_background(); }
