#include "placeholder.h"

#include "theme.h"

Placeholder::Placeholder(const QString& text, const QString& color)
    : text(text), color(color) {
  static const Theme kTheme;
  if (this->color.isEmpty()) {
    this->color = kTheme.kColorText;
  }
}

bool Placeholder::IsNull() const { return text.isEmpty(); }
