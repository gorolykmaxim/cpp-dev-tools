#ifndef PATH_H
#define PATH_H

#include <QString>

class Path {
 public:
  static QString GetFileName(const QString& path);
  static QString GetFolderPath(const QString& path);
  static bool MatchesWildcard(const QString& path, const QString& pattern);
};

#endif  // PATH_H
