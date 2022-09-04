#pragma once

#include <QSharedPointer>
#include <QString>

template<typename T>
using QPtr = QSharedPointer<T>;

const QString kQmlCurrentView = "currentView";
