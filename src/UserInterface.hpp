#pragma once

#include <QObject>
#include <QVariant>
#include <QByteArray>
#include <QSet>
#include <QHash>
#include <QList>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QString>
#include "QVariantListModel.hpp"
#include "Common.hpp"

const QString kQmlCurrentView = "currentView";
const QString kQmlInputAndListView = "InputAndListView.qml";
const QString kQmlInputAndListDataInputLabel = "inputAndListDataInputLabel";
const QString kQmlInputAndListDataInputValue = "inputAndListDataInputValue";
const QString kQmlInputAndListDataButtonText = "inputAndListDataButtonText";
const QString kQmlInputAndListDataIsButtonEnabled = "inputAndListDataIsButtonEnabled";
const QString kQmlInputAndListDataListModel = "inputAndListDataListModel";
const QHash<int, QByteArray> kQmlInputAndListDataListModelRoles = {
    {0, "title"}};
const QString kQmlInputAndListActionInputValueChanged = "inputValueChanged";
const QString kQmlInputAndListActionEnterPressed = "enterPressed";
const QString kQmlInputAndListActionItemSelected = "itemSelected";

using UserActionHandler = std::function<void(const QVariantList&)>;
using DataField = QQmlContext::PropertyPair;

struct ListField {
  QString name;
  QHash<int, QByteArray> role_names;
  QVector<QVariantList> items;
};

class UserInterface: public QObject {
  Q_OBJECT
public:
  UserInterface();
  void DisplayView(
      const QString& qml_file,
      const QList<DataField>& data_fields,
      const QList<ListField>& list_fields,
      const QHash<QString, UserActionHandler> user_action_handlers);
  void SetDataField(const QString& name, const QVariant& value);
  QVariantListModel& GetListField(const QString& name);
public slots:
  void OnUserAction(const QString& action, const QVariantList& args);
private:
  QSet<QString> data_field_names;
  QHash<QString, UserActionHandler> user_action_handlers;
  QHash<QString, QPtr<QVariantListModel>> list_fields;
  QQmlApplicationEngine engine;
};
