#include "InformationProvider.hpp"

#include <QDebug>
#include <QMetaObject>
#include <QThread>
#include <QtConcurrent>

#define EXIT_IF_SHUTDOWN() \
  if (!QCoreApplication::instance()) return;

InformationProvider::InformationProvider(QObject* parent)
    : QObject(parent), info("Hello World from provider!") {
  qDebug() << "Created";
  (void)QtConcurrent::run([this]() {
    QThread::sleep(2);
    EXIT_IF_SHUTDOWN();
    SetInfoOnUIThread("Loading...");
    QThread::sleep(2);
    EXIT_IF_SHUTDOWN();
    SetInfoOnUIThread("Preparing World...");
    QThread::sleep(2);
    EXIT_IF_SHUTDOWN();
    SetInfoOnUIThread("Constructing Hello...");
    QThread::sleep(2);
    EXIT_IF_SHUTDOWN();
    SetInfoOnUIThread("Here goes nothing...");
    QThread::sleep(2);
    EXIT_IF_SHUTDOWN();
    SetInfoOnUIThread("...");
    QThread::sleep(2);
    EXIT_IF_SHUTDOWN();
    SetInfoOnUIThread("Hello World!");
  });
}

InformationProvider::~InformationProvider() { qDebug() << "Destroyed"; }

void InformationProvider::SetInfoOnUIThread(const QString& info) {
  QMetaObject::invokeMethod(QCoreApplication::instance(), [this, info]() {
    this->info = info;
    emit infoChanged();
  });
}
