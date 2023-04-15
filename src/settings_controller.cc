#include "settings_controller.h"

#include "application.h"

SettingsController::SettingsController(QObject *parent) : QObject(parent) {
  Application::Get().view.SetWindowTitle("Settings");
  Load();
}

void SettingsController::save() {
  Application::Get().editor.SetOpenCommand(open_in_editor_command);
  Load();
}

void SettingsController::Load() {
  open_in_editor_command = Application::Get().editor.GetOpenCommand();
  emit settingsChanged();
}
