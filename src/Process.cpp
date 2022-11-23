#include "Process.hpp"

void WakeUpAndExecuteProcess(AppData& app, Process& process,
                             ProcessExecute execute,
                             const char* dbg_execute_name) {
  ProcessId id = process.id;
  if (!IsProcessAlive(app, id)) {
    qDebug() << "Attempt to resume process" << id
             << "which does not exist anymore (probably has been cancelled)";
    return;
  }
  if (execute) {
    process.execute = execute;
    process.dbg_execute_name = dbg_execute_name;
  }
  if (process.execute) {
    app.procs_to_execute.append(process.id);
  } else {
    app.procs_to_finish.append(process.id);
  }
  ExecuteProcesses(app);
}

void ExecuteProcesses(AppData& app) {
  while (!app.procs_to_execute.isEmpty() || !app.procs_to_finish.isEmpty()) {
    QList<ProcessId> execute = app.procs_to_execute;
    app.procs_to_execute.clear();
    for (ProcessId id: execute) {
      QPtr<Process> p = app.processes[id.index];
      ProcessExecute exec = p->execute;
      p->execute = nullptr;
      qDebug() << "Executing" << *p;
      exec(app);
      // Process did not specify new execute function and it has no new
      // unfinished child procesess - the process has finished.
      if (!p->execute && p->running_child_ids.isEmpty()) {
        app.procs_to_finish.append(p->id);
      }
    }
    QList<ProcessId> finish = app.procs_to_finish;
    app.procs_to_finish.clear();
    for (ProcessId id: finish) {
      QPtr<Process> p = app.processes[id.index];
      qDebug() << "Finished" << *p;
      if (p->parent_id) {
        QPtr<Process> parent = app.processes[p->parent_id.index];
        parent->running_child_ids.removeAll(p->id);
        // Wake up parent process if all children are finished
        if (parent->running_child_ids.isEmpty()) {
          if (parent->execute) {
            app.procs_to_execute.append(parent->id);
          } else {
            app.procs_to_finish.append(parent->id);
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
      for (ProcessId id: to_remove) {
        app.processes[id.index] = nullptr;
        app.procs_to_execute.removeAll(id);
        app.free_proc_ids.push(id);
      }
      p->running_child_ids.clear();
    }
  }
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

void CancelProcess(AppData& app, Process* target, Process* parent) {
  if (!target || !IsProcessAlive(app, target->id)) {
    return;
  }
  Q_ASSERT((!target->parent_id && !parent) ||
           (parent && parent->id == target->parent_id));
  qDebug() << "Cancelling" << *target;
  if (parent) {
    parent->running_child_ids.removeAll(target->id);
    // Detach target from its parent so that its parent does not get executed
    target->parent_id = ProcessId();
  }
  app.procs_to_finish.append(target->id);
}

void PrintProcesses(const AppData& app) {
  for (int i = 0; i < app.processes.size(); i++) {
    const QPtr<Process>& p = app.processes[i];
    if (p) {
      qDebug() << i << *p;
    } else {
      qDebug() << i << nullptr;
    }
  }
}
