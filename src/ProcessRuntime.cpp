#include "ProcessRuntime.hpp"
#include <QDebug>
#include <QDebugStateSaver>
#include <QVector>
#include <QtGlobal>
#include "Common.hpp"

ProcessId::ProcessId() : index(-1), version(-1) {}

ProcessId::ProcessId(int index, int version) : index(index), version(version) {}

bool ProcessId::operator==(const ProcessId& id) const {
  return index == id.index && version == id.version;
}

bool ProcessId::operator!=(const ProcessId& id) const {
  return !(*this == id);
}

ProcessId::operator bool() const {
  return index >= 0 && version >= 0;
}

QDebug operator<<(QDebug debug, const ProcessId& id) {
  QDebugStateSaver saver(debug);
  return debug.nospace() << id.index << '_' << id.version;
}

size_t qHash(const ProcessId& id, size_t seed) noexcept {
  QtPrivate::QHashCombine hash;
  seed = hash(seed, id.index);
  seed = hash(seed, id.version);
  return seed;
}

void Process::Noop(Application&) {}

void Process::KeepAlive(Application&) {
  EXEC_NEXT(KeepAlive);
}

QDebug operator<<(QDebug debug, const Process& proc) {
  QDebugStateSaver saver(debug);
  return debug.nospace() << proc.dbg_class_name << "(e="
                         << proc.dbg_execute_name << ",i=" << proc.id << ",ch="
                         << proc.running_child_ids << ')';
}

ProcessRuntime::ProcessRuntime(Application& app) : app(app) {}

void ProcessRuntime::WakeUpAndExecute(Process& process,
                                      ProcessExecute execute,
                                      const char* dbg_execute_name) {
  ProcessId id = process.id;
  if (!IsAlive(id)) {
    qDebug() << "Attempt to resume process" << id
             << "which does not exist anymore (probably has been cancelled)";
    return;
  }
  if (execute) {
    process.execute = execute;
    process.dbg_execute_name = dbg_execute_name;
  }
  if (process.execute) {
    to_execute.append(process.id);
  } else {
    to_finish.append(process.id);
  }
  ExecuteProcesses();
}

void ProcessRuntime::ExecuteProcesses() {
  while (!to_execute.isEmpty() || !to_finish.isEmpty()) {
    QVector<ProcessId> execute = to_execute;
    to_execute.clear();
    for (ProcessId id: execute) {
      QPtr<Process> p = processes[id.index];
      ProcessExecute exec = p->execute;
      p->execute = nullptr;
      qDebug() << "Executing" << *p;
      exec(app);
      // Process did not specify new execute function and it has no new
      // unfinished child procesess - the process has finished.
      if (!p->execute && p->running_child_ids.isEmpty()) {
        to_finish.append(p->id);
      }
    }
    QVector<ProcessId> finish = to_finish;
    to_finish.clear();
    for (ProcessId id: finish) {
      QPtr<Process> p = processes[id.index];
      qDebug() << "Finished" << *p;
      if (p->parent_id) {
        QPtr<Process> parent = processes[p->parent_id.index];
        parent->running_child_ids.removeAll(p->id);
        // Wake up parent process if all children are finished
        if (parent->running_child_ids.isEmpty()) {
          if (parent->execute) {
            to_execute.append(parent->id);
          } else {
            to_finish.append(parent->id);
          }
        }
      }
      // Remove all child processes
      QVector<ProcessId> to_remove = {p->id};
      QVector<ProcessId> visiting = p->running_child_ids;
      while (!visiting.isEmpty()) {
        to_remove.append(visiting);
        QVector<ProcessId> to_visit;
        for (ProcessId id: visiting) {
          to_visit.append(processes[id.index]->running_child_ids);
        }
        visiting = to_visit;
      }
      for (ProcessId id: to_remove) {
        processes[id.index] = nullptr;
        to_execute.removeAll(id);
        free_process_ids.push(id);
      }
      p->running_child_ids.clear();
    }
  }
}

bool ProcessRuntime::IsAlive(const ProcessId& id) const {
  if (!IsValid(id)) {
    return false;
  }
  QPtr<Process> p = processes[id.index];
  return p && p->id == id;
}

bool ProcessRuntime::IsValid(const ProcessId& id) const {
  return id && id.index < processes.size();
}

void ProcessRuntime::Cancel(Process* target, Process* parent) {
  if (!target || !IsAlive(target->id)) {
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
  to_finish.append(target->id);
}

void ProcessRuntime::PrintProcesses() const {
  for (int i = 0; i < processes.size(); i++) {
    const QPtr<Process>& p = processes[i];
    if (p) {
      qDebug() << i << *p;
    } else {
      qDebug() << i << nullptr;
    }
  }
}
