#include "Process.hpp"

Process::Process(const QString& command, QObject* parent)
    : QObject(parent), command(command) {}

void Process::Start() {
  QObject::connect(&process, &QProcess::readyReadStandardOutput, this, [this] {
    output += process.readAllStandardOutput();
    emit outputChanged();
  });
  QObject::connect(&process, &QProcess::readyReadStandardError, this, [this] {
    output += process.readAllStandardError();
    emit outputChanged();
  });
  QObject::connect(&process, &QProcess::errorOccurred, this,
                   [this](QProcess::ProcessError) { emit finished(); });
  QObject::connect(&process, &QProcess::finished, this,
                   [this](int, QProcess::ExitStatus) { emit finished(); });
  process.startCommand(command);
}

const QString& Process::GetOutput() const { return output; }

const QString& Process::GetCommand() const { return command; }

int Process::GetExitCode() const {
  if (process.error() == QProcess::FailedToStart ||
      process.exitStatus() == QProcess::CrashExit) {
    return -1;
  }
  return process.exitCode();
}
