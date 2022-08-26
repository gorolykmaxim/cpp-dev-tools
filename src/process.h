#pragma once

#include <functional>
#include <optional>
#include <QVector>
#include <QSet>
#include <QStack>
#include <QSharedPointer>
#include <QtGlobal>

#define EXEC(FUNC) [this] (Application& app) {FUNC(app);}
#define EXEC_NEXT(FUNC) execute = EXEC(FUNC)

class Application;

class ProcessId {
public:
  ProcessId();
  ProcessId(int index, int version);
  bool operator==(const ProcessId& id) const;
  bool operator!=(const ProcessId& id) const;
  operator bool() const;

  int index;
  int version;
};

QDebug operator<<(QDebug debug, const ProcessId& id);
size_t qHash(const ProcessId& id, size_t seed) noexcept;

using ProcessExecute = std::function<void(Application&)>;

class Process {
public:
  static const ProcessExecute kNoopExecute;

  ProcessId id;
  ProcessId parent_id;
  ProcessExecute execute;
  QVector<ProcessId> child_ids;
};

class ProcessRuntime {
public:
  explicit ProcessRuntime(Application& app);
  QSharedPointer<Process> Schedule(Process* process, Process* parent = nullptr);
  void ScheduleAndExecute(Process* process);
  void WakeUpAndExecute(Process& process, ProcessExecute execute = nullptr);
  bool IsAlive(Process& process) const;

  template<typename T>
  QSharedPointer<T> SharedPtr(T* process) const {
    Q_ASSERT(process && IsValid(process->id));
    return processes[process->id.index].template staticCast<T>();
  }

private:
  void ExecuteProcesses();
  bool AllChildrenFinished(const QSharedPointer<Process>& process) const;
  bool IsValid(const ProcessId& id) const;

  QVector<QSharedPointer<Process>> processes;
  QStack<ProcessId> free_process_ids;
  QSet<ProcessId> finished;
  QVector<ProcessId> to_execute;
  QVector<ProcessId> to_finish;
  Application& app;
};
