#ifndef CDT_H
#define CDT_H

#include <functional>
#include <optional>
#include <QList>
#include <QStack>
#include <QSet>
#include <QSharedPointer>
#include <QtGlobal>

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

size_t qHash(const ProcessId& id, size_t seed) noexcept;

using ProcessExecute = std::function<void(Application&)>;

class Process {
public:
  static const ProcessExecute kNoopExecute;

  ProcessId id;
  ProcessId parent_id;
  ProcessExecute execute;
  QSet<ProcessId> child_ids;
};

class ProcessRuntime {
public:
  explicit ProcessRuntime(Application& app);
  QSharedPointer<Process> Schedule(Process* process, Process* parent = nullptr);
  void ScheduleAndExecute(Process* process);
  void WakeUpAndExecute(Process& process, ProcessExecute execute = nullptr);
private:
  void ExecuteProcesses();

  QList<QSharedPointer<Process>> processes;
  QStack<ProcessId> free_process_ids;
  QSet<ProcessId> finished;
  QList<ProcessId> to_execute;
  QList<ProcessId> to_finish;
  Application& app;
};

class Application {
public:
  ProcessRuntime runtime;

  Application();
};

#endif
