#ifndef PLACEHOLDER_H
#define PLACEHOLDER_H

#include <QString>

struct Placeholder {
  explicit Placeholder(const QString& text = "", const QString& color = "");
  bool IsNull() const;

  QString text;
  QString color;
};

#endif  // PLACEHOLDER_H
