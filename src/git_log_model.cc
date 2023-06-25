#include "git_log_model.h"

#include "application.h"
#include "os_command.h"

static bool ParseCommitLinePart(const QString& line, QString& result,
                                int& pos) {
  int i = line.indexOf(';', pos);
  if (i < 0) {
    return false;
  }
  result = line.sliced(pos, i - pos);
  pos = i + 1;
  return true;
}

GitLogModel::GitLogModel(QObject* parent) : TextListModel(parent) {
  SetRoleNames({{0, "title"}, {1, "rightText"}});
  SetEmptyListPlaceholder("Git log is empty");
  Application::Get().view.SetWindowTitle("Git Log");
  SetPlaceholder("Loading git log...");
  OsCommand::Run("git", {"log", "--pretty=format:%h;%an;%ai;%s"}, "",
                 "Git: Failed to query git log")
      .Then(this, [this](OsProcess p) {
        if (p.exit_code == 0) {
          for (QString line : p.output.split('\n')) {
            line.remove('\r');
            int pos = 0;
            GitCommit commit;
            if (!ParseCommitLinePart(line, commit.sha, pos)) {
              continue;
            }
            if (!ParseCommitLinePart(line, commit.author, pos)) {
              continue;
            }
            if (!ParseCommitLinePart(line, commit.date, pos)) {
              continue;
            }
            commit.title = line.sliced(pos);
            list.append(commit);
          }
          LoadNew(0, list.size());
          SetPlaceholder();
        } else {
          SetPlaceholder("Failed to fetch git log", "red");
        }
      });
}

QVariantList GitLogModel::GetRow(int i) const {
  const GitCommit& c = list[i];
  return {c.title, "<b>" + c.author + "</b> " + c.date};
}

int GitLogModel::GetRowCount() const { return list.size(); }
