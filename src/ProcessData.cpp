#include "ProcessData.hpp"
#include "Process.hpp"

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

void Process::Noop(AppData&) {}

void Process::KeepAlive(AppData&) {
  EXEC_NEXT(KeepAlive);
}

QDebug operator<<(QDebug debug, const Process& proc) {
  QDebugStateSaver saver(debug);
  return debug.nospace() << proc.dbg_class_name << "(e="
                         << proc.dbg_execute_name << ",i=" << proc.id << ",ch="
                         << proc.running_child_ids << ')';
}
