#include "DisplayExec.hpp"

#include "Process.hpp"
#include "Task.hpp"
#include "UI.hpp"

#define LOG() qDebug() << "[DisplayExec]"

static const QString kSubTextColor = "#6d6d6d";

DisplayExec::DisplayExec() { EXEC_NEXT(Display); }

void DisplayExec::Display(AppData& app) {
  AutoReSelectExec(app);
  UpdateAndDrawSelectedExec(app, true);
  QHash<int, QByteArray> role_names = {{0, "title"},     {1, "subTitle"},
                                       {2, "id"},        {3, "icon"},
                                       {4, "iconColor"}, {5, "isSelected"}};
  DisplayView(
      app, kViewSlot, "ExecView.qml",
      {UIDataField{"windowTitle", "Task Execution History"},
       UIDataField{"vExecFilter", ""},
       UIDataField{"vExecCmd", selected_exec_cmd},
       UIDataField{"vExecStatus", selected_exec_status},
       UIDataField{"vExecOutput", selected_exec_output},
       UIDataField{"vExecOutputFilter", ""},
       UIDataField{"vExecOutputSearchResults", "No Results"},
       UIDataField{"vExecOutputNoSearchResults", true},
       UIDataField{"vExecOutputSearchResultStart", 0},
       UIDataField{"vExecOutputSearchResultEnd", 0},
       UIDataField{"vExecsEmpty", app.execs.isEmpty()}},
      {UIListField{"vExecs", role_names, MakeFilteredListOfExecs(app)}});
  WakeUpProcessOnUIEvent(app, kViewSlot, "execHistoryChanged", *this,
                         EXEC(this, ReDrawExecHistory));
  WakeUpProcessOnUIEvent(app, kViewSlot, "execOutputChanged", *this,
                         EXEC(this, ReDrawExecOutput));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaExecFilterChanged", *this,
                         EXEC(this, FilterExecs));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaExecSelected", *this,
                         EXEC(this, SelectExec));
  WakeUpProcessOnUIEvent(app, kViewSlot, "vaSearchExecOutput", *this,
                         EXEC(this, SearchExecOutput));
}

void DisplayExec::ReDrawExecHistory(AppData& app) {
  if (select_new_execs) {
    if (AutoReSelectExec(app)) {
      ResetSearchResults(app);
    }
    UpdateAndDrawSelectedExec(app);
  }
  QList<QVariantList> execs = MakeFilteredListOfExecs(app);
  SetUIDataField(app, kViewSlot, "vExecsEmpty", app.execs.isEmpty());
  GetUIListField(app, kViewSlot, "vExecs").SetItems(execs);
  EXEC_NEXT(KeepAlive);
}

void DisplayExec::ReDrawExecOutput(AppData& app) {
  EXEC_NEXT(KeepAlive);
  QUuid id = GetEventArg(app, 0).toUuid();
  if (selected_exec_id != id) {
    return;
  }
  UpdateAndDrawSelectedExec(app);
}

void DisplayExec::FilterExecs(AppData& app) {
  exec_filter = GetEventArg(app, 0).toString();
  ReDrawExecHistory(app);
}

void DisplayExec::SelectExec(AppData& app) {
  selected_exec_id = GetEventArg(app, 0).toUuid();
  Exec* exec = FindLatestRunningExec(app);
  bool is_latest_running = exec && exec->id == selected_exec_id;
  bool is_latest =
      !app.execs.isEmpty() && app.execs.last().id == selected_exec_id;
  select_new_execs = is_latest_running || is_latest;
  if (select_new_execs) {
    LOG() << "Latest execution will get selected:" << selected_exec_id;
  } else {
    LOG() << "Execution selected:" << selected_exec_id;
  }
  ResetSearchResults(app);
  UpdateAndDrawSelectedExec(app);
  EXEC_NEXT(KeepAlive);
}

void DisplayExec::ResetSearchResults(AppData& app) {
  LOG() << "Resetting execution output search results";
  SetUIDataField(app, kViewSlot, "vExecOutputFilter", "");
  SearchExecOutput(app, "");
}

void DisplayExec::SearchExecOutput(AppData& app) {
  EXEC_NEXT(KeepAlive);
  QString term = GetEventArg(app, 0).toString().toHtmlEscaped();
  SearchExecOutput(app, term);
}

void DisplayExec::SearchExecOutput(AppData& app, const QString& search_term) {
  current_search_result = 0;
  search_results.clear();
  if (search_term.size() > 2) {
    LOG() << "Searching execution output for" << search_term;
    qsizetype pos = 0;
    while (true) {
      qsizetype i =
          selected_exec_output.indexOf(search_term, pos, Qt::CaseInsensitive);
      if (i < 0) {
        break;
      }
      ExecOutputSearchResult result;
      result.start = i;
      result.end = i + search_term.size();
      search_results.append(result);
      pos = result.end + 1;
    }
    LOG() << "Found" << search_results.size() << "search results";
  }
  QString summary;
  ExecOutputSearchResult result;
  if (search_results.isEmpty()) {
    summary = "No Results";
  } else {
    summary = QString::number(current_search_result + 1) + " of " +
              QString::number(search_results.size());
    result = search_results[current_search_result];
  }
  SetUIDataField(app, kViewSlot, "vExecOutputSearchResults", summary);
  SetUIDataField(app, kViewSlot, "vExecOutputNoSearchResults",
                 search_results.isEmpty());
  SetUIDataField(app, kViewSlot, "vExecOutputSearchResultStart", result.start);
  SetUIDataField(app, kViewSlot, "vExecOutputSearchResultEnd", result.end);
}

bool DisplayExec::AutoReSelectExec(AppData& app) {
  QUuid old = selected_exec_id;
  if (Exec* exec = FindLatestRunningExec(app)) {
    selected_exec_id = exec->id;
  } else if (!app.execs.isEmpty()) {
    selected_exec_id = app.execs.last().id;
  }
  return old != selected_exec_id;
}

void DisplayExec::UpdateAndDrawSelectedExec(AppData& app, bool only_update) {
  Exec* exec = FindExecById(app, selected_exec_id);
  if (!exec) {
    selected_exec_cmd.clear();
    selected_exec_status.clear();
    selected_exec_output.clear();
  } else {
    selected_exec_cmd = ShortenTaskCmd(exec->cmd, *app.current_project);
    selected_exec_status = "Running...";
    if (exec->exit_code) {
      selected_exec_status = "Exit Code: " + QString::number(*exec->exit_code);
    }
    selected_exec_output = exec->output.toHtmlEscaped();
    selected_exec_output.replace("\n", "<br>");
  }
  if (only_update) {
    return;
  }
  SetUIDataField(app, kViewSlot, "vExecCmd", selected_exec_cmd);
  SetUIDataField(app, kViewSlot, "vExecStatus", selected_exec_status);
  SetUIDataField(app, kViewSlot, "vExecOutput", selected_exec_output);
}

QList<QVariantList> DisplayExec::MakeFilteredListOfExecs(AppData& app) {
  QList<QVariantList> rows;
  for (const Exec& exec : app.execs) {
    QString icon, iconColor;
    if (!exec.exit_code) {
      if (!exec.proc) {
        icon = "access_alarm";
        iconColor = kSubTextColor;
      } else {
        icon = "autorenew";
        iconColor = "green";
      }
    } else {
      if (*exec.exit_code == 0) {
        icon = "check";
        iconColor = "";
      } else {
        icon = "error";
        iconColor = "red";
      }
    }
    QString cmd = ShortenTaskCmd(exec.cmd, *app.current_project);
    bool is_selected = exec.id == selected_exec_id;
    AppendToUIListIfMatches(rows, exec_filter,
                            {cmd, exec.start_time.toString(), exec.id, icon,
                             iconColor, is_selected},
                            {0, 1});
  }
  return rows;
}
