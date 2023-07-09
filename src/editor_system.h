#ifndef EDITORSYSTEM_H
#define EDITORSYSTEM_H

#include <QObject>

class EditorSystem : public QObject {
  Q_OBJECT
 public:
  void Initialize();
  void OpenFile(const QString& file, int line, int col);

  QString open_command;
};

#endif  // EDITORSYSTEM_H
