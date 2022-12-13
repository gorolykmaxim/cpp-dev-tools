#pragma once

#include "Lib.hpp"

struct AppData;

struct ProcessId {
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

using ProcessExecute = std::function<void(AppData&)>;

enum ProcessFlags {
  kProcessIgnoreEventsUntilNextWakeUp = 1,
  kProcessCancelled = 2,
};

struct Process {
  void Noop(AppData& app);
  void KeepAlive(AppData& app);

  ProcessId id;
  ProcessId parent_id;
  ProcessExecute execute, cancel;
  QList<ProcessId> running_child_ids;
  int flags = 0;
  const char* dbg_class_name = nullptr;
  const char* dbg_execute_name = nullptr;
};

QDebug operator<<(QDebug debug, const Process& proc);

struct ProcessWakeUpCall {
  ProcessId id;
  ProcessExecute execute;
  const char* dbg_execute_name = nullptr;
};

struct Event {
  Event(const QString& type, const QVariantList& args);

  QString type;
  QVariantList args;
};

QDebug operator<<(QDebug debug, const Event& event);

struct DeferredExecuteChange {
  Process* target = nullptr;
  ProcessExecute execute;
  const char* dbg_class_name = nullptr;
  const char* dbg_execute_name = nullptr;

  ~DeferredExecuteChange();
};
