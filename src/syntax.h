#ifndef SYNTAX_H
#define SYNTAX_H

#include "text_area_controller.h"

class Syntax {
 public:
  static QList<TextFormat> FindStringsAndComments(
      const QString &text, const QString &comment_symbol);
  static bool SectionsContain(const QList<TextFormat> &sections, int i);
};

using SyntaxFormatFunc = std::function<QList<TextFormat>(const QString &text)>;

class SyntaxFormatter : public TextFormatter {
 public:
  explicit SyntaxFormatter(QObject *parent = nullptr);
  QList<TextFormat> Format(const QString &text, LineInfo line) const;
  void DetectLanguageByFile(const QString &file_name);

 private:
  SyntaxFormatFunc format;
};

#endif  // SYNTAX_H
