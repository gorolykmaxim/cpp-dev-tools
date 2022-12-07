#pragma once

#include "AppData.hpp"

void InitializeUI(AppData& app);
void DisplayView(AppData& app, const QString& slot_name,
                 const QString& qml_file, const QList<UIDataField>& data_fields,
                 const QList<UIListField>& list_fields);
void SetUIDataField(AppData& app, const QString& slot_name,
                    const QString& name, const QVariant& value);
QVariantListModel& GetUIListField(AppData& app, const QString& slot_name,
                                  const QString& name);
void DisplayAlertDialog(AppData& app, const QString& title,
                        const QString& text, bool error = true,
                        bool cancellable = false);
void DisplayStatusBar(AppData& app);
void DisplayMenuBar(AppData& app);
