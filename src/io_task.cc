#include "io_task.h"

#include "application.h"

BaseIoTask::BaseIoTask() : QObject(nullptr) {}

void BaseIoTask::Run() {
  (void)QtConcurrent::run(&Application::Get().io_thread_pool, [this] {
    RunInBackground();
    emit finished();
    deleteLater();
  });
}

class VoidIoTask : public BaseIoTask {
 public:
  explicit VoidIoTask(const std::function<void()> &on_background)
      : BaseIoTask(), on_background(on_background) {}

 protected:
  void RunInBackground() { on_background(); }

 private:
  std::function<void()> on_background;
};

void IoTask::Run(QObject *requestor, const std::function<void()> &on_io_thread,
                 const std::function<void()> &on_ui_thread) {
  VoidIoTask *task = new VoidIoTask(on_io_thread);
  QObject::connect(task, &BaseIoTask::finished, requestor,
                   [on_ui_thread] { on_ui_thread(); });
  task->Run();
}
