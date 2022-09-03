#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

class UserInterface: public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantMap variant MEMBER variant NOTIFY VariantChanged)
signals:
  void VariantChanged();
public:
  void SetVariant(const QString& name, const QVariant& value);

  QVariantMap variant;
};
