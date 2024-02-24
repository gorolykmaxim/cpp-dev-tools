#include "git_show_model.h"

#include "application.h"
#include "os_command.h"
#include "theme.h"

#define LOG() qDebug() << "[GitShowModel]"

static const QString kMoveSign = " => ";

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

static QString NormalizePathIfMove(const QString& path) {
  int start_i = path.indexOf('{');
  int move_i = path.indexOf(kMoveSign);
  int end_i = path.indexOf('}');
  if (start_i < 0 || move_i < 0 || end_i < 0 || !(start_i < move_i < end_i)) {
    return path;
  }
  QString prefix = path.sliced(0, start_i);
  QString suffix = end_i + 1 < path.size() ? path.sliced(end_i + 1) : "";
  QStringList old_and_new = path.sliced(start_i + 1, end_i - start_i - 1).split(kMoveSign);
  return prefix + old_and_new[0] + suffix + kMoveSign + prefix + old_and_new[1] + suffix;
}

void GitShowModel::FetchCommitInfo() {
  OsCommand::Run(
      "git", {"show", "--format=commit %H%nAuthor: %an %ae%nDate: %ad%n%n%B",
              "--numstat", sha})
      .Then(this, [this](OsProcess p) {
        bool error = p.exit_code != 0;
        if (p.exit_code == 0) {
          int i = p.output.lastIndexOf("\n\n\n");
          if (i < 0) {
            LOG() << "Failed to parse git show:" << p.output;
            error = true;
          } else {
            raw_commit_info = p.output.sliced(0, i);
            QString numstat = p.output.sliced(i + 3);
            QStringList lines = numstat.split('\n', Qt::SkipEmptyParts);
            for (const QString &line : lines) {
              QStringList parts = line.split('\t');
              if (parts.size() < 3) {
                error = true;
                LOG() << "Failed to parse git show numstat line:" << line;
                break;
              }
              QString path = NormalizePathIfMove(parts[2]);
              files.append({path, parts[0].toInt(), parts[1].toInt()});
            }
            Load();
            emit changeListChanged();
          }
        }
        if (error) {
          SetPlaceholder("Failed to get details about " + sha, "red");
        } else {
          SetPlaceholder();
        }
      });
}

void GitShowModel::DiffSelectedFile() {
  QString path = GetSelectedFileName();
  path = path.contains(kMoveSign) ? path.split(kMoveSign)[1] : path;
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
