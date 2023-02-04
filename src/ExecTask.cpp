#include "ExecTask.hpp"

#include "Process.hpp"
#include "Task.hpp"

#define LOG() qDebug() << "[ExecTask]"

ExecTask::ExecTask(const QString& cmd) : cmd(cmd) { EXEC_NEXT(Start); }

void ExecTask::Start(AppData& app) {
  Exec exec;
  exec_id = exec.id = QUuid::createUuid();
  exec.cmd = cmd;
  exec.start_time = QDateTime::currentDateTime();
  exec.proc = QPtr<QProcess>::create();
  app.execs.append(exec);
  LOG() << "Starting" << exec;
  QPtr<ExecTask> self = ProcessSharedPtr(app, this);
  QPtr<ExecOSCmd> task_proc =
      ScheduleProcess<ExecOSCmd>(app, this, exec.cmd, exec.proc);
  task_proc->on_output = [&app, self](const QString& data) {
    if (Exec* current = FindExecById(app, self->exec_id)) {
      current->output += data;
      app.events.enqueue(Event("execOutputChanged", {self->exec_id}));
      ExecuteProcesses(app);
    }
  };
  app.events.enqueue(Event("execHistoryChanged", {}));
  EXEC_NEXT(Finish);
}

void ExecTask::Finish(AppData& app) {
  Exec* exec = FindExecById(app, exec_id);
  if (!exec) {
    return;
  }
  QProcess& proc = *exec->proc;
  if (proc.error() == QProcess::FailedToStart ||
      proc.exitStatus() == QProcess::CrashExit) {
    exec->exit_code = -1;
  } else {
    exec->exit_code = proc.exitCode();
  }
  app.events.enqueue(Event("execHistoryChanged", {}));
  LOG() << "Finished" << *exec;
}
