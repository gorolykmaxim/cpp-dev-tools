#pragma once

#include <QObject>
#include <QtQmlIntegration>

class Theme : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(int basePadding MEMBER kBasePadding CONSTANT)
  Q_PROPERTY(int baseRadius MEMBER kBaseRadius CONSTANT)
  Q_PROPERTY(int centeredViewWidth MEMBER kCenteredViewWidth CONSTANT)
  Q_PROPERTY(QString colorText MEMBER kColorText CONSTANT)
  Q_PROPERTY(QString colorBackground MEMBER kColorBackground CONSTANT)
  Q_PROPERTY(QString colorBorder MEMBER kColorBorder CONSTANT)
  Q_PROPERTY(QString colorPrimary MEMBER kColorPrimary CONSTANT)
  Q_PROPERTY(QString colorPlaceholder MEMBER kColorPlaceholder CONSTANT)
  Q_PROPERTY(QString colorCurrentLine MEMBER kColorCurrentLine CONSTANT)
  Q_PROPERTY(QString colorTextSelection MEMBER kColorTextSelection CONSTANT)
  Q_PROPERTY(
      QString colorButtonPrimaryLight MEMBER kColorButtonPrimaryLight CONSTANT)
  Q_PROPERTY(
      QString colorButtonPrimaryDark MEMBER kColorButtonPrimaryDark CONSTANT)
  Q_PROPERTY(QString colorButtonBorder MEMBER kColorButtonBorder CONSTANT)
  Q_PROPERTY(QString colorSearchResult MEMBER kColorSearchResult CONSTANT)
 public:
  const int kBasePadding = 4;
  const int kBaseRadius = 4;
  const int kCenteredViewWidth = 500;
  const QString kColorText = "#efefef";
  const QString kColorBackground = "#232527";
  const QString kColorBorder = "#3a3b3b";
  const QString kColorPrimary = "#3784ff";
  const QString kColorPlaceholder = "#9fa0a0";
  const QString kColorCurrentLine = "#2f3138";
  const QString kColorTextSelection = "#3f638b";
  const QString kColorButtonPrimaryLight = "#3182e6";
  const QString kColorButtonPrimaryDark = "#2a6dcc";
  const QString kColorButtonBorder = "#1c1a19";
  const QString kColorSearchResult = "#6b420f";
};
