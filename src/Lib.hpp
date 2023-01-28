#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDebugStateSaver>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMetaObject>
#include <QModelIndex>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQueue>
#include <QQuickStyle>
#include <QSet>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStack>
#include <QStandardPaths>
#include <QString>
#include <QThreadPool>
#include <QUrl>
#include <QUuid>
#include <QVariant>
#include <QtConcurrent>
#include <algorithm>
#include <chrono>
#include <functional>
#include <limits>
#include <optional>
#include <utility>

template <typename T>
using QPtr = QSharedPointer<T>;
