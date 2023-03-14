#pragma once

#include <QObject>
#include <QtQmlIntegration>

class Theme : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(int basePadding MEMBER kBasePadding CONSTANT)
  Q_PROPERTY(int baseRadius MEMBER kBaseRadius CONSTANT)
  Q_PROPERTY(QString colorBgBright MEMBER kColorBgBright CONSTANT)
  Q_PROPERTY(QString colorBgLight MEMBER kColorBgLight CONSTANT)
  Q_PROPERTY(QString colorBgMedium MEMBER kColorBgMedium CONSTANT)
  Q_PROPERTY(QString colorBgDark MEMBER kColorBgDark CONSTANT)
  Q_PROPERTY(QString colorBgBlack MEMBER kColorBgBlack CONSTANT)
  Q_PROPERTY(QString colorText MEMBER kColorText CONSTANT)
  Q_PROPERTY(QString colorSubText MEMBER kColorSubText CONSTANT)
  Q_PROPERTY(QString colorHighlight MEMBER kColorHighlight CONSTANT)
 public:
  const int kBasePadding = 4;
  const int kBaseRadius = 4;
  const QString kColorBgBright = "#6a6a6a";
  const QString kColorBgLight = "#585859";
  const QString kColorBgMedium = "#383838";
  const QString kColorBgDark = "#282828";
  const QString kColorBgBlack = "#191919";
  const QString kColorText = "#efefef";
  const QString kColorSubText = "#6d6d6d";
  const QString kColorHighlight = "#9ab8ef";
};
