#include <QDebug>
#include <QDebugStateSaver>
#include "ProcessRuntime.hpp"

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

QDebug operator<<(QDebug debug, const Process& proc) {
  QDebugStateSaver saver(debug);
  return debug.nospace() << proc.dbg_class_name << "(e="
                         << proc.dbg_execute_name << ",i=" << proc.id << ')';
}

ProcessRuntime::ProcessRuntime(Application& app) : app(app) {}

void ProcessRuntime::WakeUpAndExecute(Process& process,
                                      ProcessExecute execute,
                                      const char* dbg_execute_name) {
  ProcessId id = process.id;
  Q_ASSERT(IsValid(id));
  if (processes[id.index]->id != id) {
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
      if (!p->execute && AllChildrenFinished(p)) {
        to_finish.append(p->id);
      }
    }
    QVector<ProcessId> finish = to_finish;
    to_finish.clear();
    for (ProcessId id: finish) {
      QPtr<Process> p = processes[id.index];
      qDebug() << "Finished" << *p;
      finished.insert(p->id);
      // Wake up parent process if all children are finished
      if (p->parent_id) {
        QPtr<Process> parent = processes[p->parent_id.index];
        if (AllChildrenFinished(parent)) {
          if (parent->execute) {
            to_execute.append(parent->id);
          } else {
            to_finish.append(parent->id);
          }
        }
      }
      // Remove all child processes
      QVector<ProcessId> to_remove;
      if (!p->parent_id) {
        // If current process is a root and have finished - remove it since
        // nobody else is interested in its results.
        to_remove.append(p->id);
      }
      QVector<ProcessId> visiting = p->child_ids;
      while (!visiting.isEmpty()) {
        to_remove.append(visiting);
        QVector<ProcessId> to_visit;
        for (ProcessId id: visiting) {
          to_visit.append(processes[id.index]->child_ids);
        }
        visiting = to_visit;
      }
      for (ProcessId id: to_remove) {
        processes[id.index] = nullptr;
        free_process_ids.push(id);
        finished.remove(id);
      }
      p->child_ids.clear();
    }
  }
}

bool ProcessRuntime::IsAlive(Process& process) const {
  if (!IsValid(process.id)) {
    return false;
  }
  QPtr<Process> p = processes[process.id.index];
  return p && p->id == process.id;
}

bool ProcessRuntime::AllChildrenFinished(const QPtr<Process>& process) const {
  Q_ASSERT(process);
  for (ProcessId id: process->child_ids) {
    if (!finished.contains(id)) {
      return false;
    }
  }
  return true;
}

bool ProcessRuntime::IsValid(const ProcessId& id) const {
  return id && id.index < processes.size() && processes[id.index] != nullptr;
}

const ProcessExecute Process::kNoopExecute = [] (Application& app) {};
