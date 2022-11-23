#pragma once

#include "Application.hpp"

void InitializeUI(Application& app);
void DisplayView(
    Application& app, const QString& slot_name, const QString& qml_file,
    const QList<UIDataField>& data_fields,
    const QList<UIListField>& list_fields,
    const QHash<QString, UserActionHandler>& user_action_handlers);
void SetUIDataField(Application& app, const QString& slot_name,
                    const QString& name, const QVariant& value);
QVariantListModel& GetUIListField(Application& app, const QString& slot_name,
                                  const QString& name);
void DisplayAlertDialog(Application& app, const QString& title,
                        const QString& text, bool error = true,
                        bool cancellable = false,
                        const std::function<void()>& on_ok = nullptr,
                        const std::function<void()>& on_cancel = nullptr);
void DisplayStatusBar(Application& app, const QVector<QVariantList>& itemsLeft,
                      const QVector<QVariantList>& itemsRight);
