#include "AlertDialog.hpp"

void AlertDialogDisplay(UserInterface& ui, const QString& title,
                        const QString& text, bool error, bool cancellable,
                        const std::function<void()>& on_ok,
                        const std::function<void()>& on_cancel) {
  UserActionHandler ok_handler = [on_ok] (const QVariantList&) {
    if (on_ok) {
      on_ok();
    }
  };
  UserActionHandler cancel_handler = [on_cancel] (const QVariantList&) {
    if (on_cancel) {
      on_cancel();
    }
  };
  ui.DisplayView(
      kDialogSlot,
      "AlertDialog.qml",
      {
        DataField{"dataDialogVisible", true},
        DataField{"dataDialogTitle", title},
        DataField{"dataDialogText", text},
        DataField{"dataDialogCancellable", cancellable},
        DataField{"dataDialogError", error},
      },
      {},
      {
        {"actionDialogOk", ok_handler},
        {"actionDialogCancel", cancel_handler},
      });
}
