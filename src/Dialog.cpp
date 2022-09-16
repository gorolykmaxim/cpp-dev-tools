#include "Dialog.hpp"

void Dialog::DisplayError(const QString& title, const QString& text,
                          UserInterface& ui) {
  ui.SetDataField("dialogDataTitle", title);
  ui.SetDataField("dialogDataText", text);
  ui.SetDataField("dialogDataButtonText", "OK");
  ui.SetDataField("dialogDataVisible", true);
}
