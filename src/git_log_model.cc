#include "git_log_model.h"

#include "application.h"
#include "database.h"
#include "os_command.h"

#define LOG() qDebug() << "[GitLogModel]"

static bool ParseCommitLinePart(const QString& line, QString& result, int& pos,
                                bool& error) {
  int i = line.indexOf(';', pos);
  if (i < 0) {
    error = true;
    return false;
  }
  result = line.sliced(pos, i - pos);
  pos = i + 1;
  if (pos >= line.size()) {
    error = true;
    return false;
  }
  return true;
}

GitLogModel::GitLogModel(QObject* parent) : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "rightText"}});
  SetEmptyListPlaceholder("Git log is empty");
  Application& app = Application::Get();
  connect(&app.git, &GitSystem::pull, this, [this] { load(); });
  QUuid project_id = app.project.GetCurrentProject().id;
  Database::LoadState(this,
                      "SELECT branch_or_file, search_term FROM "
                      "git_log_context WHERE project_id=?",
                      {project_id}, [this](QVariantList result) {
                        if (!result.isEmpty()) {
                          branch_or_file = result[0].toString();
                          search_term = result[1].toString();
                          emit optionsChanged();
                        }
                        load();
                      });
}

QString GitLogModel::GetSelectedCommitSha() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? "" : list[i].sha;
}

void GitLogModel::setBranchOrFile(const QString& value) {
  branch_or_file = value;
  SaveOptions();
}

void GitLogModel::setSearchTerm(const QString& value) {
  search_term = value;
  SaveOptions();
}

void GitLogModel::load(bool only_reselect) {
  if (only_reselect) {
    ReSelectItem(-1);
    return;
  }
  list.clear();
  SetPlaceholder("Loading git log...");
  QStringList args = {"log", "--pretty=format:%h;%an;%ai;%s"};
  LOG() << "Loading git log. search_term:" << search_term
        << "branch/file:" << branch_or_file;
  if (!search_term.isEmpty()) {
    args.append({"--grep", search_term});
  }
  if (!branch_or_file.isEmpty()) {
    args.append(branch_or_file);
  }
  OsCommand::Run("git", args, "", "Git: Failed to query git log")
      .Then(this, [this](OsProcess p) {
        bool error = p.exit_code != 0;
        if (p.exit_code == 0) {
          for (const QString& line : p.output.split('\n', Qt::SkipEmptyParts)) {
            int pos = 0;
            GitCommit commit;
            if (!ParseCommitLinePart(line, commit.sha, pos, error)) {
              break;
            }
            if (!ParseCommitLinePart(line, commit.author, pos, error)) {
              break;
            }
            if (!ParseCommitLinePart(line, commit.date, pos, error)) {
              break;
            }
            commit.title = line.sliced(pos);
            list.append(commit);
          }
          LOG() << "Loaded git log of" << list.size() << "commits";
          Load();
        }
        if (error) {
          SetPlaceholder("Failed to fetch git log:\n" + p.output, "red");
        } else {
          SetPlaceholder();
        }
      });
}

void GitLogModel::checkout() {
  QString sha = GetSelectedCommitSha();
  if (sha.isEmpty()) {
    return;
  }
  LOG() << "Checking-out commit" << sha;
  ExecuteGitCommand({"checkout", sha}, "Git: Failed to checkout " + sha);
}

void GitLogModel::cherryPick() {
  QString sha = GetSelectedCommitSha();
  if (sha.isEmpty()) {
    return;
  }
  LOG() << "Cherry-picking commit" << sha;
  ExecuteGitCommand({"cherry-pick", sha}, "Git: Failed to cherry-pick " + sha);
}

void GitLogModel::reset(bool hard) {
  QString sha = GetSelectedCommitSha();
  if (sha.isEmpty()) {
    return;
  }
  LOG() << "Resetting (hard:" << hard << ") current bracn to commit" << sha;
  ExecuteGitCommand({"reset", hard ? "--hard" : "--soft", sha},
                    "Git: Failed to reset to commit " + sha);
}

QVariantList GitLogModel::GetRow(int i) const {
  const GitCommit& c = list[i];
  return {c.title, "<b>" + c.author + "</b> " + c.date};
}

int GitLogModel::GetRowCount() const { return list.size(); }

void GitLogModel::SaveOptions() const {
  QUuid project_id = Application::Get().project.GetCurrentProject().id;
  Database::ExecCmdAsync(
      "INSERT OR REPLACE INTO git_log_context VALUES(?, ?, ?)",
      {project_id, branch_or_file, search_term});
}

void GitLogModel::ExecuteGitCommand(const QStringList& args,
                                    const QString& error_title) {
  OsCommand::Run("git", args, "", error_title)
      .Then(this, [this](OsProcess p) {
        if (p.exit_code == 0) {
          load();
        }
      });
}
