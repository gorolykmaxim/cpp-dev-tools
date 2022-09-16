#include "Dialog.hpp"
#include <QVariant>
#include <QString>
#include "UserInterface.hpp"

void DialogInit(UserInterface& ui) {
  ui.SetDataField("dialogDataTitle", QVariant());
  ui.SetDataField("dialogDataText", QVariant());
  ui.SetDataField("dialogDataButtonText", QVariant());
  ui.SetDataField("dialogDataVisible", QVariant());
}

void DialogDisplayError(const QString& title, const QString& text,
                        UserInterface& ui) {
  ui.SetDataField("dialogDataTitle", title);
  ui.SetDataField("dialogDataText", text);
  ui.SetDataField("dialogDataButtonText", "OK");
  ui.SetDataField("dialogDataVisible", true);
}
