#ifndef IOTASK_H
#define IOTASK_H

#include <QObject>
#include <QtConcurrent>

class BaseIoTask : public QObject {
  Q_OBJECT
 public:
  explicit BaseIoTask(QObject* parent = nullptr);
  void Run();

 protected:
  virtual void RunInBackground() = 0;

 signals:
  void finished();
};

template <typename T>
class TemplateIoTask : public BaseIoTask {
 public:
  explicit TemplateIoTask(const std::function<T()>& on_background,
                          QObject* parent)
      : BaseIoTask(parent), on_background(on_background) {}

  T result;

 protected:
  void RunInBackground() { result = on_background(); }

 private:
  std::function<T()> on_background;
};

class IoTask {
 public:
  template <typename T>
  static void Run(QObject* requestor, const std::function<T()>& on_io_thread,
                  const std::function<void(T)>& on_ui_thread) {
    auto task = new TemplateIoTask<T>(on_io_thread, requestor);
    QObject::connect(task, &BaseIoTask::finished, requestor,
                     [task, on_ui_thread] {
                       on_ui_thread(task->result);
                       task->deleteLater();
                     });
    task->Run();
  }

  static void Run(QObject* requestor, const std::function<void()>& on_io_thread,
                  const std::function<void()>& on_ui_thread);
};

#endif  // IOTASK_H
