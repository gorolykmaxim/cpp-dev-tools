#pragma once

#include <functional>
#include <QObject>
#include <QString>
#include <QQmlContext>
#include "QVariantListModel.hpp"

class InputAndListView: public QObject {
  Q_OBJECT
  Q_PROPERTY(QString inputValue MEMBER input_value NOTIFY InputValueChanged)
  Q_PROPERTY(QString inputLabel MEMBER input_label NOTIFY LabelsChanged)
  Q_PROPERTY(QString buttonText MEMBER button_text NOTIFY LabelsChanged)
  Q_PROPERTY(bool isButtonEnabled MEMBER is_button_enabled
             NOTIFY IsButtonEnabledChanged)
  Q_PROPERTY(QVariantListModel* listModel READ GetListModel CONSTANT)
signals:
  void LabelsChanged();
  void InputValueChanged();
  void IsButtonEnabledChanged();
public slots:
  void OnEnter();
  void OnListItemSelected(int index);
  void OnValueChanged(const QString& value);
public:
  explicit InputAndListView(QQmlContext* context);
  void Display(const QString& input_label, const QString& button_text);
  void SetValue(const QString& value);
  void SetButtonEnabled(bool value);
  QVariantListModel* GetListModel();

  QString input_value, input_label, button_text;
  bool is_button_enabled = false;
  std::function<void()> on_enter;
  std::function<void(const QString&)> on_value_changed;
  std::function<void(int)> on_list_item_selected;
  QVariantListModel list_model;
private:
  QQmlContext* context;
};
