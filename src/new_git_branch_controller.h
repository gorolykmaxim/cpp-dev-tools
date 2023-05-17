#ifndef NEWGITBRANCHCONTROLLER_H
#define NEWGITBRANCHCONTROLLER_H

#include <QObject>
#include <QQmlEngine>

class NewGitBranchController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString branchBasis MEMBER branch_basis NOTIFY branchBasisChanged)
  Q_PROPERTY(QString error MEMBER error NOTIFY errorChanged)
 public:
  explicit NewGitBranchController(QObject* parent = nullptr);

 public slots:
  void createBranch(const QString& name);

 signals:
  void branchBasisChanged();
  void errorChanged();
  void success();

 private:
  QString branch_basis;
  QString error;
};

#endif  // NEWGITBRANCHCONTROLLER_H
