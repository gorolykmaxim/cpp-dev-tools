#pragma once

#include <QObject>
#include <QString>
#include <QtQmlIntegration>

class InformationProvider : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString info MEMBER info NOTIFY infoChanged)
  QML_ELEMENT
 public:
  explicit InformationProvider(QObject* parent = nullptr);
  ~InformationProvider();

  QString info;

 private:
  void SetInfoOnUIThread(const QString& info);
 signals:
  void infoChanged();
};
