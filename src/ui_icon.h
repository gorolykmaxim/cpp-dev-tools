#pragma once

#include <QString>

struct UiIcon {
  UiIcon(const QString& icon = "", const QString& color = "")
      : icon(icon), color(color) {}

  QString icon;
  QString color;
};
