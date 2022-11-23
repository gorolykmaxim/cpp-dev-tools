#pragma once

#include <chrono>
#include <functional>
#include <limits>
#include <utility>
#include <algorithm>

#include <QString>
#include <QVariant>
#include <QHash>
#include <QByteArray>
#include <QList>
#include <QSet>
#include <QStack>
#include <QUrl>
#include <QSharedPointer>

#include <QJsonDocument>
#include <QJsonObject>

#include <QDir>
#include <QStandardPaths>
#include <QFile>

#include <QtConcurrent>
#include <QThreadPool>

#include <QDebug>
#include <QDebugStateSaver>

#include <QAbstractListModel>
#include <QModelIndex>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QMetaObject>
#include <QQuickStyle>

template<typename T>
using QPtr = QSharedPointer<T>;
