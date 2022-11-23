#pragma once

#include "AppData.hpp"

void InitializeUI(AppData& app);
void DisplayView(
    AppData& app, const QString& slot_name, const QString& qml_file,
    const QList<UIDataField>& data_fields,
    const QList<UIListField>& list_fields,
    const QHash<QString, UserActionHandler>& user_action_handlers);
void SetUIDataField(AppData& app, const QString& slot_name,
                    const QString& name, const QVariant& value);
QVariantListModel& GetUIListField(AppData& app, const QString& slot_name,
                                  const QString& name);
void DisplayAlertDialog(AppData& app, const QString& title,
                        const QString& text, bool error = true,
                        bool cancellable = false,
                        const std::function<void()>& on_ok = nullptr,
                        const std::function<void()>& on_cancel = nullptr);
void DisplayStatusBar(AppData& app, const QVector<QVariantList>& itemsLeft,
                      const QVector<QVariantList>& itemsRight);
