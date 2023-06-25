#include "git_show_model.h"

#include "application.h"
#include "os_command.h"
#include "theme.h"

#define LOG() qDebug() << "[GitShowModel]"

GitShowModel::GitShowModel(QObject *parent) : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "rightText"}});
  Application::Get().view.SetWindowTitle("Git Show");
  connect(this, &TextListModel::selectedItemChanged, this,
          &GitShowModel::DiffSelectedFile);
  connect(this, &GitShowModel::shaChanged, this,
          &GitShowModel::FetchCommitInfo);
}

int GitShowModel::CalSidebarWidth() const {
  return ViewSystem::CalcWidthInMonoFont(QString(50, 'a')) +
         Theme().kBasePadding;
}

QString GitShowModel::GetStats() const {
  int additions = 0;
  int removals = 0;
  for (const GitShowFile &f : files) {
    additions += f.additions;
    removals += f.removals;
  }
  return GitSystem::FormatChangeStats(additions, removals);
}

bool GitShowModel::HasChanges() const { return !files.isEmpty(); }

QString GitShowModel::GetSelectedFileName() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? "" : files[i].path;
}

QVariantList GitShowModel::GetRow(int i) const {
  const GitShowFile &f = files[i];
  return {f.path, GitSystem::FormatChangeStats(f.additions, f.removals)};
}

int GitShowModel::GetRowCount() const { return files.size(); }

void GitShowModel::FetchCommitInfo() {
  OsCommand::Run(
      "git", {"show", "--format=commit %H%nAuthor: %an %ae%nDate: %ad%n%n%B",
              "--numstat", sha})
      .Then(this, [this](OsProcess p) {
        if (p.exit_code == 0) {
          int i = p.output.lastIndexOf("\n\n");
          raw_commit_info = p.output.sliced(0, i + 1);
          QString numstat = p.output.sliced(i);
          for (const QString &line : numstat.split('\n', Qt::SkipEmptyParts)) {
            QStringList parts = line.split('\t');
            files.append({parts[2], parts[0].toInt(), parts[1].toInt()});
          }
          Load();
          emit changeListChanged();
          SetPlaceholder();
        } else {
          SetPlaceholder("Failed to get details about " + sha, "red");
        }
      });
}

void GitShowModel::DiffSelectedFile() {
  QString path = GetSelectedFileName();
  if (path.isEmpty()) {
    LOG() << "Clearing git diff view";
    SetDiff("", "");
    return;
  }
  LOG() << "Will get git diff of" << path;
  OsCommand::Run("git", {"show", "--format=", sha, "--", path}, "",
                 "Git: Failed to get git diff")
      .Then(this, [this, path](OsProcess p) {
        if (p.exit_code == 0) {
          SetDiff(p.output, "");
        } else {
          SetDiff("", "Failed to git diff '" + path + '\'');
        }
      });
}

void GitShowModel::SetDiff(const QString &diff, const QString &error) {
  raw_diff = diff;
  diff_error = error;
  emit rawDiffChanged();
}
