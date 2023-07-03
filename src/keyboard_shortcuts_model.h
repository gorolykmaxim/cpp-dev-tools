#ifndef KEYBOARDSHORTCUTSMODEL_H
#define KEYBOARDSHORTCUTSMODEL_H

#include <QQmlEngine>

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

 public slots:
  void selectCurrentCommand();
  void load();

 protected:
  QVariantList GetRow(int i) const;
  int GetRowCount() const;

 signals:
  void selectedCommandChanged();

 private:
  QList<UserCommand> list;
  int selected_command;
};

#endif  // KEYBOARDSHORTCUTSMODEL_H
