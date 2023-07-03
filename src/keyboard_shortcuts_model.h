#ifndef KEYBOARDSHORTCUTSMODEL_H
#define KEYBOARDSHORTCUTSMODEL_H

#include <QQmlEngine>

#include "database.h"
#include "text_list_model.h"
#include "user_command_system.h"

class KeyboardShortcutsModel : public TextListModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString selectedCommand READ GetSelectedCommand NOTIFY
                 selectedCommandChanged)
  Q_PROPERTY(QString selectedShortcut READ GetSelectedShortcut NOTIFY
                 selectedCommandChanged)
 public:
  explicit KeyboardShortcutsModel(QObject* parent);
  QString GetSelectedCommand() const;
  QString GetSelectedShortcut() const;
  QList<Database::Cmd> MakeCommandsToUpdateDatabase() const;
  void ResetAllModifications();

 public slots:
  void selectCurrentCommand();
  void load();
  void setSelectedShortcut(const QString& shortcut);
  void resetCurrentShortcut();
  void resetAllShortcuts();
  void restoreDefaultOfCurrentShortcut();

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;

 signals:
  void selectedCommandChanged();

 private:
  QList<UserCommand> list;
  QHash<int, QString> new_shortcut;
  int selected_command;
};

#endif  // KEYBOARDSHORTCUTSMODEL_H
