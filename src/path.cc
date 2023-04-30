#include "path.h"

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
