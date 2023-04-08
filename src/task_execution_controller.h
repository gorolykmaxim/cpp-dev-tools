#pragma once

#include <QObject>
#include <QQuickTextDocument>
#include <QSyntaxHighlighter>
#include <QtQmlIntegration>

class TaskExecutionOutputHighlighter : public QSyntaxHighlighter {
 public:
  TaskExecutionOutputHighlighter();

  QSet<int> stderr_line_indices;

 protected:
  void highlightBlock(const QString& text) override;

 private:
  QTextCharFormat error_line_format;
};

class TaskExecutionController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      QString executionName MEMBER execution_name NOTIFY executionChanged)
  Q_PROPERTY(
      QString executionStatus MEMBER execution_status NOTIFY executionChanged)
  Q_PROPERTY(
      QString executionIcon MEMBER execution_icon NOTIFY executionChanged)
  Q_PROPERTY(QString executionIconColor MEMBER execution_icon_color NOTIFY
                 executionChanged)
  Q_PROPERTY(
      QString executionOutput MEMBER execution_output NOTIFY executionChanged)
 public:
  explicit TaskExecutionController(QObject* parent = nullptr);

 public slots:
  void AttachTaskExecutionOutputHighlighter(QQuickTextDocument* document);

 signals:
  void executionChanged();

 private:
  void LoadExecution(bool include_output);

  QString execution_name;
  QString execution_status;
  QString execution_output;
  QString execution_icon;
  QString execution_icon_color;
  TaskExecutionOutputHighlighter execution_output_highlighter;
};
