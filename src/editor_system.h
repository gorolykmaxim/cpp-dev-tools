#ifndef EDITORSYSTEM_H
#define EDITORSYSTEM_H

#include <QObject>

class EditorSystem : public QObject {
  Q_OBJECT
 public:
  void Initialize();
  void OpenFile(const QString& file, int column, int row);

  QString open_command;
};

#endif  // EDITORSYSTEM_H
