#include "path.h"

#include <QStringList>

QString Path::GetFileName(const QString& path) {
  int pos = path.lastIndexOf('/');
  if (pos < 0) {
    return path;
  } else if (pos == path.size() - 1) {
    return "";
  } else {
    return path.sliced(pos + 1);
  }
}

QString Path::GetFolderPath(const QString& path) {
  int pos = path.lastIndexOf('/');
  if (pos <= 0) {
    return "/";
  } else {
    return path.sliced(0, pos + 1);
  }
}

bool Path::MatchesWildcard(const QString& path, const QString& pattern) {
  QStringList parts = pattern.split('*');
  int pos = 0;
  for (int i = 0; i < parts.size(); i++) {
    const QString& part = parts[i];
    if (part.isEmpty()) {
      continue;
    }
    int j = path.indexOf(part, pos, Qt::CaseSensitive);
    if (j < 0) {
      return false;
    }
    if (i == 0 && j != 0) {
      return false;
    } else if (i == parts.size() - 1 && part.size() + j != path.size()) {
      return false;
    }
    pos = part.size() + j + 1;
  }
  return true;
}
