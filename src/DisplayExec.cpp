#include "DisplayExec.hpp"

#include "Process.hpp"
#include "UI.hpp"

DisplayExec::DisplayExec() { EXEC_NEXT(Display); }

void DisplayExec::Display(AppData &app) {
  QHash<int, QByteArray> role_names = {
      {0, "title"},     {1, "subTitle"},   {2, "id"},        {3, "icon"},
      {4, "iconColor"}, {5, "titleColor"}, {6, "isSelected"}};
  DisplayView(app, kViewSlot, "ExecView.qml",
              {UIDataField{"windowTitle", "Task Executions"},
               UIDataField{"vExecFilter", ""}, UIDataField{"vExecIcon", ""},
               UIDataField{"vExecIconColor", ""}, UIDataField{"vExecName", ""},
               UIDataField{"vExecCmd", ""}, UIDataField{"vExecOutput", ""}},
              {UIListField{"vExecs", role_names, {}}});
  EXEC_NEXT(Noop);
}
