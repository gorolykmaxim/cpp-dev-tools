#ifndef GITSYSTEM_H
#define GITSYSTEM_H

#include <QObject>

class GitSystem : public QObject {
  Q_OBJECT
 public:
  static QList<QString> FindIgnoredPathsSync();
  static void Pull();
};

#endif  // GITSYSTEM_H
