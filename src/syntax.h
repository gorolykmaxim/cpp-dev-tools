#ifndef SYNTAX_H
#define SYNTAX_H

#include "text_area_controller.h"

class Syntax {
 public:
  static QList<TextSectionFormat> FindStringsAndComments(
      const QString &text, const QString &comment_symbol);
  static bool SectionsContain(const QList<TextSectionFormat> &sections, int i);
};

class SyntaxFormatter : public TextAreaFormatter {
 public:
  using FormatFunc =
      std::function<QList<TextSectionFormat>(const QString &text)>;

  explicit SyntaxFormatter(QObject *parent = nullptr);
  QList<TextSectionFormat> Format(const QString &text, const QTextBlock &block);
  void DetectLanguageByFile(const QString &file_name);

 private:
  FormatFunc format;
};

#endif  // SYNTAX_H
