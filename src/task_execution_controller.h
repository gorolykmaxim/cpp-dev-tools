#pragma once

#include <QObject>
#include <QtQmlIntegration>

#include "text_area_controller.h"

class TaskExecutionOutputFormatter : public TextAreaFormatter {
 public:
  explicit TaskExecutionOutputFormatter(QObject* parent);
  QList<TextSectionFormat> Format(const QString& text,
                                  const QTextBlock& block) override;

  QSet<int> stderr_line_indicies;
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
  Q_PROPERTY(
      TextAreaFormatter* executionFormatter MEMBER execution_formatter CONSTANT)
 public:
  explicit TaskExecutionController(QObject* parent = nullptr);

 signals:
  void executionChanged();

 private:
  void LoadExecution(bool include_output);

  QString execution_name;
  QString execution_status;
  QString execution_output;
  QString execution_icon;
  QString execution_icon_color;
  TaskExecutionOutputFormatter* execution_formatter;
};
