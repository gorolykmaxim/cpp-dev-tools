#include "InformationProvider.hpp"

#include <QDebug>
#include <QMetaObject>
#include <QThread>
#include <QtConcurrent>

InformationProvider::InformationProvider(QObject* parent)
    : QObject(parent), info("Hello World from provider!") {
  qDebug() << "Created";
  (void)QtConcurrent::run([this]() {
    QThread::sleep(2);
    SetInfoOnUIThread("Loading...");
    QThread::sleep(2);
    SetInfoOnUIThread("Preparing World...");
    QThread::sleep(2);
    SetInfoOnUIThread("Constructing Hello...");
    QThread::sleep(2);
    SetInfoOnUIThread("Here goes nothing...");
    QThread::sleep(2);
    SetInfoOnUIThread("...");
    QThread::sleep(2);
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
