#pragma once

#include <QObject>
#include <QString>
#include <QQmlContext>

class InputAndListView: public QObject {
  Q_OBJECT
  Q_PROPERTY(QString title MEMBER title NOTIFY TitleChanged)
public:
  explicit InputAndListView(QQmlContext* context);
  void Display(const QString& title);

  QString title;
signals:
  void TitleChanged();
private:
  QQmlContext* context;
};

class UserInterface {
public:
  UserInterface(QQmlContext* context);

  InputAndListView input_and_list;
};
