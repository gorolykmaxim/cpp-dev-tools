#ifndef EDITORSYSTEM_H
#define EDITORSYSTEM_H

#include <QObject>

class EditorSystem : public QObject {
  Q_OBJECT
 public:
  void Initialize();
  void SetOpenCommand(const QString& cmd);
  QString GetOpenCommand() const;
  void OpenFile(const QString& file);

 private:
  QString open_command;
};

#endif  // EDITORSYSTEM_H
