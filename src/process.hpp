#pragma once

#include <functional>
#include <optional>
#include <QVector>
#include <QSet>
#include <QStack>
#include <QtGlobal>
#include "common.hpp"

#define EXEC(FUNC) [this] (Application& app) {FUNC(app);}
#define EXEC_NEXT(FUNC)\
  execute = EXEC(FUNC);\
  dbg_execute_name = #FUNC;\
  dbg_class_name = dbg_class_name ? dbg_class_name : __FUNCTION__

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
  const char* dbg_class_name = nullptr;
  const char* dbg_execute_name = nullptr;
};

QDebug operator<<(QDebug debug, const Process& proc);

class ProcessRuntime {
public:
  explicit ProcessRuntime(Application& app);

  template<typename P, typename... Args>
  QPtr<P> ScheduleRoot(Args&&... args) {
    QPtr<P> p(new P(args...));
    if (!free_process_ids.isEmpty()) {
      p->id = free_process_ids.pop();
      p->id.version++;
      processes[p->id.index] = p;
    } else {
      p->id = ProcessId(processes.size(), 0);
      processes.append(p);
    }
    to_execute.append(p->id);
    return p;
  }

  template<typename P, typename... Args>
  QPtr<P> Schedule(Process* parent, Args&&... args) {
    QPtr<P> p = ScheduleRoot<P>(args...);
    if (parent) {
      p->parent_id = parent->id;
      parent->child_ids.append(p->id);
    }
    return p;
  }

  template<typename P, typename... Args>
  QPtr<P> ScheduleAndExecute(Args&&... args) {
    QPtr<P> p = ScheduleRoot<P>(args...);
    ExecuteProcesses();
    return p;
  }

  void WakeUpAndExecute(Process& process, ProcessExecute execute = nullptr);
  bool IsAlive(Process& process) const;

  template<typename T>
  QPtr<T> SharedPtr(T* process) const {
    Q_ASSERT(process && IsValid(process->id));
    return processes[process->id.index].template staticCast<T>();
  }

private:
  void ExecuteProcesses();
  bool AllChildrenFinished(const QPtr<Process>& process) const;
  bool IsValid(const ProcessId& id) const;

  QVector<QPtr<Process>> processes;
  QStack<ProcessId> free_process_ids;
  QSet<ProcessId> finished;
  QVector<ProcessId> to_execute;
  QVector<ProcessId> to_finish;
  Application& app;
};
