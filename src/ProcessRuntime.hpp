#pragma once

#include "Base.hpp"

#define EXEC(PROC, FUNC) [PROC] (Application& app) {PROC->FUNC(app);}, #FUNC
#define EXEC_STATIC(PROC, FUNC) [PROC] (Application& app) {FUNC(app);}, #FUNC
#define EXEC_NEXT(FUNC)\
  execute = [this] (Application& app) {FUNC(app);};\
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
  void Noop(Application& app);
  void KeepAlive(Application& app);

  ProcessId id;
  ProcessId parent_id;
  ProcessExecute execute;
  QVector<ProcessId> running_child_ids;
  const char* dbg_class_name = nullptr;
  const char* dbg_execute_name = nullptr;
};

QDebug operator<<(QDebug debug, const Process& proc);

class ProcessRuntime {
public:
  explicit ProcessRuntime(Application& app);

  template<typename P, typename... Args>
  QPtr<P> Schedule(Process* parent, Args&&... args) {
    QPtr<P> p = QPtr<P>::create(args...);
    Q_ASSERT(p->execute);
    if (!free_process_ids.isEmpty()) {
      p->id = free_process_ids.pop();
      p->id.version++;
      processes[p->id.index] = p;
    } else {
      p->id = ProcessId(processes.size(), 0);
      processes.append(p);
    }
    to_execute.append(p->id);
    if (parent) {
      Q_ASSERT(IsAlive(parent->id));
      p->parent_id = parent->id;
      parent->running_child_ids.append(p->id);
    }
    return p;
  }

  template<typename P, typename... Args>
  QPtr<P> ReSchedule(Process* target, Process* parent, Args&&... args) {
    Cancel(target, parent);
    return Schedule<P>(parent, args...);
  }

  template<typename P, typename... Args>
  QPtr<P> ScheduleAndExecute(Process* parent, Args&&... args) {
    QPtr<P> p = Schedule<P>(parent, args...);
    ExecuteProcesses();
    return p;
  }

  template<typename P, typename... Args>
  QPtr<P> ReScheduleAndExecute(Process* target, Process* parent,
                               Args&&... args) {
    Cancel(target, parent);
    return ScheduleAndExecute<P>(parent, args...);
  }

  void WakeUpAndExecute(Process& process, ProcessExecute execute = nullptr,
                        const char* dbg_execute_name = nullptr);
  bool IsAlive(const ProcessId& id) const;

  template<typename T>
  QPtr<T> SharedPtr(T* process) const {
    Q_ASSERT(process && IsAlive(process->id));
    return processes[process->id.index].template staticCast<T>();
  }

private:
  void ExecuteProcesses();
  bool IsValid(const ProcessId& id) const;
  void Cancel(Process* target, Process* parent);
  void PrintProcesses() const; // To be called from debugger

  QVector<QPtr<Process>> processes;
  QStack<ProcessId> free_process_ids;
  QVector<ProcessId> to_execute;
  QVector<ProcessId> to_finish;
  Application& app;
};
