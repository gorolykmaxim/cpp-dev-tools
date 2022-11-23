#pragma once

#include "AppData.hpp"

#define EXEC(PROC, FUNC) [PROC] (AppData& app) {PROC->FUNC(app);}, #FUNC
#define EXEC_STATIC(PROC, FUNC) [PROC] (AppData& app) {FUNC(app);}, #FUNC
#define EXEC_NEXT(FUNC)\
  execute = [this] (AppData& app) {FUNC(app);};\
  dbg_execute_name = #FUNC;\
  dbg_class_name = dbg_class_name ? dbg_class_name : __FUNCTION__

void ExecuteProcesses(AppData& app);
bool IsProcessValid(const AppData& app, const ProcessId& id);
void CancelProcess(AppData& app, Process* target, Process* parent);
void WakeUpAndExecuteProcess(AppData& app, Process& process,
                             ProcessExecute execute = nullptr,
                             const char* dbg_execute_name = nullptr);
bool IsProcessAlive(const AppData& app, const ProcessId& id);
void PrintProcesses(const AppData& app); // To be called from debugger

template<typename P, typename... Args>
QPtr<P> ScheduleProcess(AppData& app, Process* parent, Args&&... args) {
  QPtr<P> p = QPtr<P>::create(args...);
  Q_ASSERT(p->execute);
  if (!app.free_proc_ids.isEmpty()) {
    p->id = app.free_proc_ids.pop();
    p->id.version++;
    app.processes[p->id.index] = p;
  } else {
    p->id = ProcessId(app.processes.size(), 0);
    app.processes.append(p);
  }
  app.procs_to_execute.append(p->id);
  if (parent) {
    Q_ASSERT(IsProcessAlive(app, parent->id));
    p->parent_id = parent->id;
    parent->running_child_ids.append(p->id);
  }
  return p;
}

template<typename P, typename... Args>
QPtr<P> ReScheduleProcess(AppData& app, Process* target, Process* parent,
                   Args&&... args) {
  CancelProcess(app, target, parent);
  return ScheduleProcess<P>(app, parent, args...);
}

template<typename P, typename... Args>
QPtr<P> ScheduleAndExecuteProcess(AppData& app, Process* parent,
                                  Args&&... args) {
  QPtr<P> p = ScheduleProcess<P>(app, parent, args...);
  ExecuteProcesses(app);
  return p;
}

template<typename P, typename... Args>
QPtr<P> ReScheduleAndExecuteProcess(AppData& app, Process* target,
                                    Process* parent, Args&&... args) {
  CancelProcess(app, target, parent);
  return ScheduleAndExecuteProcess<P>(app, parent, args...);
}

template<typename T>
QPtr<T> ProcessSharedPtr(const AppData& app, T* process) {
  Q_ASSERT(process && IsProcessAlive(app, process->id));
  return app.processes[process->id.index].template staticCast<T>();
}
