#include "git_log_model.h"

#include "application.h"
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
  connect(&Application::Get().git, &GitSystem::pull, this,
          [this] { load(-1); });
}

QString GitLogModel::GetSelectedCommitSha() const {
  int i = GetSelectedItemIndex();
  return i < 0 ? "" : list[i].sha;
}

void GitLogModel::setBranchOrFile(const QString& value) {
  branch_or_file = value;
}

void GitLogModel::load(int item_to_select) {
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
      .Then(this, [this, item_to_select](OsProcess p) {
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
          Load(item_to_select);
        }
        if (error) {
          SetPlaceholder("Failed to fetch git log:\n" + p.output, "red");
        } else {
          SetPlaceholder();
        }
      });
}

void GitLogModel::checkout() {
  int i = GetSelectedItemIndex();
  if (i < 0) {
    return;
  }
  const GitCommit& commit = list[i];
  LOG() << "Checking-out commit" << commit.sha;
  OsCommand::Run("git", {"checkout", commit.sha}, "",
                 "Git: Failed to checkout " + commit.sha)
      .Then(this, [this](OsProcess p) {
        if (p.exit_code == 0) {
          load();
        }
      });
}

void GitLogModel::cherryPick() {
  int i = GetSelectedItemIndex();
  if (i < 0) {
    return;
  }
  const GitCommit& commit = list[i];
  LOG() << "Cherry-picking commit" << commit.sha;
  OsCommand::Run("git", {"cherry-pick", commit.sha}, "",
                 "Git: Failed to cherry-pick " + commit.sha)
      .Then(this, [this](OsProcess p) {
        if (p.exit_code == 0) {
          load();
        }
      });
}

QVariantList GitLogModel::GetRow(int i) const {
  const GitCommit& c = list[i];
  return {c.title, "<b>" + c.author + "</b> " + c.date};
}

int GitLogModel::GetRowCount() const { return list.size(); }
