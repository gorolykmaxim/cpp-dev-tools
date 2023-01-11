#pragma once

#include <chrono>
#include <functional>
#include <limits>
#include <utility>
#include <algorithm>
#include <optional>

#include <QString>
#include <QVariant>
#include <QHash>
#include <QByteArray>
#include <QList>
#include <QQueue>
#include <QSet>
#include <QStack>
#include <QUrl>
#include <QSharedPointer>
#include <QDateTime>
#include <QUuid>

#include <QJsonDocument>
#include <QJsonObject>

#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QProcess>

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
