#include "Process.hpp"

#define EXEC_NEXT_OBJ(PROC, FUNC)\
  PROC.execute = [&PROC] (AppData& app) {PROC.FUNC(app);};\
  PROC.dbg_execute_name = #FUNC

#define LOG() qDebug() << "[Process]"

static void WakeUpProcess(AppData& app, Process& process,
                          ProcessExecute execute = nullptr,
                          const char* dbg_execute_name = nullptr) {
  if (execute) {
    process.execute = execute;
    process.dbg_execute_name = dbg_execute_name;
  }
  if (process.execute) {
    app.procs_to_execute.insert(process.id);
  } else {
    app.procs_to_finish.insert(process.id);
  }
}

void WakeUpAndExecuteProcess(AppData& app, Process& process,
                             ProcessExecute execute,
                             const char* dbg_execute_name) {
  ProcessId id = process.id;
  if (!IsProcessAlive(app, id)) {
    LOG() << "Attempt to resume process" << id
          << "which does not exist anymore (probably has been cancelled)";
    return;
  }
  WakeUpProcess(app, process, execute, dbg_execute_name);
  ExecuteProcesses(app);
}

void WakeUpProcessOnEvent(AppData& app, const QString& event_type,
                          Process& process, ProcessExecute execute,
                          const char* dbg_execute_name) {
  Q_ASSERT(IsProcessAlive(app, process.id));
  ProcessWakeUpCall call;
  call.id = process.id;
  call.execute = execute;
  call.dbg_execute_name = dbg_execute_name;
  app.event_listeners[event_type].append(call);
  LOG() << "Process" << process.id << "starts to listen to" << event_type
        << "events";
  EXEC_NEXT_OBJ(process, Noop);
}

void WakeUpProcessOnUIEvent(AppData& app, const QString& slot_name,
                            const QString& event_type, Process& process,
                            ProcessExecute execute,
                            const char* dbg_execute_name) {
  WakeUpProcessOnEvent(app, event_type, process, execute, dbg_execute_name);
  app.view_data[slot_name].event_types.insert(event_type);
}

QVariant GetEventArg(const AppData& app, int i) {
  Q_ASSERT(!app.events.isEmpty());
  return app.events.head().args[i];
}

void ExecuteProcesses(AppData& app) {
  do {
    bool dequeue_event_later = false;
    if (!app.events.isEmpty()) {
      dequeue_event_later = true;
      Event& event = app.events.head();
      LOG() << "Handling" << event;
      QList<ProcessWakeUpCall>& calls = app.event_listeners[event.type];
      for (ProcessWakeUpCall& call: calls) {
        QPtr<Process> p = app.processes[call.id.index];
        if (p->flags & kProcessIgnoreEventsUntilNextWakeUp) {
          continue;
        }
        WakeUpProcess(app, *p, call.execute, call.dbg_execute_name);
        LOG() << "Waking up" << *p;
      }
    }
    while (!app.procs_to_execute.isEmpty() || !app.procs_to_finish.isEmpty()) {
      QSet<ProcessId> finish = app.procs_to_finish;
      app.procs_to_finish.clear();
      for (ProcessId id: finish) {
        QPtr<Process> p = app.processes[id.index];
        if (!p || p->id != id) {
          LOG() << "Process" << id << "already finished";
          continue;
        }
        LOG() << "Finished" << *p;
        if (p->parent_id) {
          QPtr<Process> parent = app.processes[p->parent_id.index];
          parent->running_child_ids.removeAll(p->id);
          // Wake up parent process if all children are finished
          if (parent->running_child_ids.isEmpty()) {
            if (parent->execute) {
              app.procs_to_execute.insert(parent->id);
            } else {
              app.procs_to_finish.insert(parent->id);
            }
          }
        }
        // Remove all child processes
        QList<ProcessId> to_remove = {p->id};
        QList<ProcessId> visiting = p->running_child_ids;
        while (!visiting.isEmpty()) {
          to_remove.append(visiting);
          QList<ProcessId> to_visit;
          for (ProcessId id: visiting) {
            to_visit.append(app.processes[id.index]->running_child_ids);
          }
          visiting = to_visit;
        }
        bool is_cancelled = p->flags & kProcessCancelled;
        // Traversing it backwards to execute "cancel" functions of the
        // child processes before the parent process.
        for (auto it = to_remove.rbegin(); it != to_remove.rend(); it++) {
          ProcessId& id = *it;
          QPtr<Process> proc = app.processes[id.index];
          if (is_cancelled && proc->cancel) {
            proc->cancel(app);
          }
          app.processes[id.index] = nullptr;
          app.procs_to_execute.remove(id);
          app.free_proc_ids.push(id);
          app.named_processes.removeIf([id] (const auto& it) {
            return it.value() == id;
          });
          for (const QString& event_type: app.event_listeners.keys()) {
            QList<ProcessWakeUpCall>& calls = app.event_listeners[event_type];
            calls.removeIf([id] (const ProcessWakeUpCall& c) {
              return c.id == id;
            });
          }
        }
        p->running_child_ids.clear();
      }
      QSet<ProcessId> execute = app.procs_to_execute;
      app.procs_to_execute.clear();
      for (ProcessId id: execute) {
        QPtr<Process> p = app.processes[id.index];
        ProcessExecute exec = p->execute;
        p->execute = nullptr;
        p->flags &= ~kProcessIgnoreEventsUntilNextWakeUp;
        LOG() << "Executing" << *p;
        exec(app);
        // Process did not specify new execute function and it has no new
        // unfinished child procesess - the process has finished.
        if (!p->execute && p->running_child_ids.isEmpty()) {
          app.procs_to_finish.insert(p->id);
        }
      }
    }
    if (dequeue_event_later) {
      app.events.dequeue();
    }
  } while (!app.events.isEmpty());
}

bool IsProcessAlive(const AppData& app, const ProcessId& id) {
  if (!IsProcessValid(app, id)) {
    return false;
  }
  QPtr<Process> p = app.processes[id.index];
  return p && p->id == id;
}

bool IsProcessValid(const AppData& app, const ProcessId& id) {
  return id && id.index < app.processes.size();
}

static void CancelProcess(AppData& app, Process& target) {
  LOG() << "Cancelling" << target;
  if (IsProcessAlive(app, target.parent_id)) {
    QPtr<Process> parent = app.processes[target.parent_id.index];
    parent->running_child_ids.removeAll(target.id);
    // Detach target from its parent so that its parent does not get executed
    target.parent_id = ProcessId();
  }
  target.flags |= kProcessCancelled;
  app.procs_to_finish.insert(target.id);
}

void CancelProcess(AppData& app, const ProcessId& target) {
  if (!IsProcessAlive(app, target)) {
    return;
  }
  CancelProcess(app, *app.processes[target.index]);
}

void CancelProcess(AppData& app, Process* target) {
  if (!target || !IsProcessAlive(app, target->id)) {
    return;
  }
  CancelProcess(app, *target);
}

void CancelAllNamedProcesses(AppData& app) {
  for (const QString& name: app.named_processes.keys()) {
    CancelProcess(app, app.named_processes[name]);
  }
}

void PrintProcesses(const AppData& app) {
  for (int i = 0; i < app.processes.size(); i++) {
    const QPtr<Process>& p = app.processes[i];
    if (p) {
      LOG() << i << *p;
    } else {
      LOG() << i << nullptr;
    }
  }
}
