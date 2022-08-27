#include <QDebug>
#include "process.h"

ProcessId::ProcessId(): index(-1), version(-1) {}

ProcessId::ProcessId(int index, int version): index(index), version(version) {}

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
  debug.nospace() << "Id(i=" << id.index << ",v="
                  << id.version << ')';
  return debug.space();
}

size_t qHash(const ProcessId& id, size_t seed) noexcept {
  QtPrivate::QHashCombine hash;
  seed = hash(seed, id.index);
  seed = hash(seed, id.version);
  return seed;
}

ProcessRuntime::ProcessRuntime(Application& app): app(app) {}

void ProcessRuntime::WakeUpAndExecute(Process& process,
                                      ProcessExecute execute) {
  ProcessId id = process.id;
  Q_ASSERT(IsValid(id));
  if (processes[id.index]->id != id) {
    qDebug() << "Attempt to resume process" << id
             << "which does not exist anymore (probably has been cancelled)";
    return;
  }
  if (execute) {
    process.execute = execute;
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
      qDebug() << "Executing process" << p->id;
      exec(app);
      // Process did not specify new execute function and it has no new
      // unfinished child procesess - the process has finished.
      if (!p->execute && AllChildrenFinished(p)) {
        to_finish.append(p->id);
      }
    }
    for (ProcessId id: to_finish) {
      QPtr<Process> p = processes[id.index];
      qDebug() << "Finishing process" << p->id;
      finished.insert(p->id);
      // Wake up parent process if all children are finished
      if (p->parent_id && AllChildrenFinished(processes[p->parent_id.index])) {
        to_execute.append(p->parent_id);
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
    to_finish.clear();
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
  return id && id.index < processes.size();
}

const ProcessExecute Process::kNoopExecute = [] (Application& app) {};
