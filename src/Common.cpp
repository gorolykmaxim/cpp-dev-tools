#include "Common.hpp"

bool HighlighSubstring(QString& str, const QString& sub_str) {
  if (sub_str.isEmpty()) {
    return false;
  }
  int i = str.toLower().indexOf(sub_str.toLower());
  if (i >= 0) {
    str.insert(i + sub_str.size(), "</b>");
    str.insert(i, "<b>");
    return true;
  } else {
    return false;
  }
}
