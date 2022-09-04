#include "Initialize.hpp"
#include <QStandardPaths>
#include <QString>
#include <QJsonDocument>
#include <QDebug>

Initialize::Initialize() {
  EXEC_NEXT(ReadConfig);
}

void Initialize::ReadConfig(Application& app) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  QString user_config_path = home + "/.cpp-dev-tools.json";
  read_config = app.runtime.Schedule<JsonFileProcess>(this,
                                                      JsonOperation::kRead,
                                                      user_config_path);
  EXEC_NEXT(LoadConfig);
}

void Initialize::LoadConfig(Application& app) {
  app.user_config.LoadFrom(read_config->json);
  app.runtime.Schedule<JsonFileProcess>(this, JsonOperation::kWrite,
                                        read_config->path,
                                        app.user_config.Save());
  EXEC_NEXT(DisplayUi);
}

void Initialize::DisplayUi(Application& app) {
  QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  app.ui.input_and_list.SetValue(home);
  app.ui.input_and_list.on_value_changed = [] (const QString& value) {
    qDebug() << "new value" << value;
  };
  app.ui.input_and_list.on_list_item_selected = [] (int index) {
    qDebug() << "new item" << index;
  };
  app.ui.input_and_list.Display("Open project by path:", "Open");
}
