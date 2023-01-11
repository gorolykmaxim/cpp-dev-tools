#include "ExecTask.hpp"
#include "Process.hpp"
#include "ExecOSCmd.hpp"

ExecTask::ExecTask(const QString& task_name) : task_name(task_name) {
  EXEC_NEXT(Schedule);
}

static Task* GetTaskByName(AppData& app, const QString& name) {
  Task* result = nullptr;
  for (Task& task: app.tasks) {
    if (task.name == name) {
      result = &task;
      break;
    }
  }
  Q_ASSERT(result);
  return result;
}

static void ScheduleTask(AppData& app, const Task& task, QUuid id,
                         QUuid primary_exec_id) {
  Exec exec;
  exec.id = id;
  exec.primary_exec_id = primary_exec_id;
  exec.task_name = task.name;
  exec.cmd = task.cmd;
  qDebug() << "Scheduling" << exec;
  app.execs.append(exec);
}

static Exec* FindExecById(AppData& app, QUuid id) {
  for (Exec& exec: app.execs) {
    if (exec.id == id) {
      return &exec;
    }
  }
  return nullptr;
}

void ExecTask::Schedule(AppData& app) {
  Task* primary = GetTaskByName(app, task_name);
  primary_exec_id = QUuid::createUuid();
  for (const QString& name: primary->pre_tasks) {
    ScheduleTask(app, *GetTaskByName(app, name), QUuid::createUuid(),
                 primary_exec_id);
  }
  ScheduleTask(app, *primary, primary_exec_id, primary_exec_id);
  ExecNext(app);
}

void ExecTask::ExecNext(AppData& app) {
  bool pre_task_failed = false;
  for (Exec& exec: app.execs) {
    if (primary_exec_id != exec.primary_exec_id) {
      continue;
    }
    if (exec.proc && !exec.exit_code) {
      // Process has finished but the execution has not been updated yet.
      if (exec.proc->error() == QProcess::FailedToStart ||
          exec.proc->exitStatus() == QProcess::CrashExit) {
        exec.exit_code = -1;
      } else {
        exec.exit_code = exec.proc->exitCode();
      }
      qDebug() << "Finishing" << exec;
    }
    if (exec.exit_code) {
      // Execution is finished.
      if (*exec.exit_code != 0) {
        pre_task_failed = true;
      }
    } else {
      // Execution has not been started yet.
      exec.start_time = QDateTime::currentDateTime();
      if (pre_task_failed) {
        qDebug() << "Aborting due to failed pre task" << exec;
        exec.exit_code = -1;
      } else {
        exec.proc = QPtr<QProcess>::create();
        qDebug() << "Starting" << exec;
        QPtr<ExecOSCmd> p = ScheduleProcess<ExecOSCmd>(app, this, exec.cmd,
                                                       exec.proc);
        QUuid id = exec.id;
        p->on_output = [&app, id] (const QString& data) {
          if (Exec* exec = FindExecById(app, id)) {
            exec->output += data;
            qDebug() << data;
          }
        };
        EXEC_NEXT(ExecNext);
        break;
      }
    }
  }
}
