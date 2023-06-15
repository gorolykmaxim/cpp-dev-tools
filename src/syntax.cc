#include "syntax.h"

#include "view_system.h"

using Regex = QRegularExpression;

static QTextCharFormat TextColorToFormat(const QString &hex) {
  QTextCharFormat format;
  format.setForeground(ViewSystem::BrushFromHex(hex));
  return format;
}

static bool CompareLength(const QString &a, const QString &b) {
  return a.size() > b.size();
}

static Regex KeywordsRegExp(
    QStringList words, Regex::PatternOptions options = Regex::NoPatternOption) {
  std::sort(words.begin(), words.end(), CompareLength);
  return Regex("\\b" + words.join("\\b|\\b") + "\\b", options);
}

static QRegularExpression KeywordsRegExpNoBoundaries(QStringList words) {
  std::sort(words.begin(), words.end(), CompareLength);
  return QRegularExpression(words.join('|'));
}

static const Regex kNumberRegex("\\b([0-9.]+|0x[0-9.a-f]+)\\b");
static const QTextCharFormat kFunctionNameFormat = TextColorToFormat("#dcdcaa");
static const QTextCharFormat kCommentFormat = TextColorToFormat("#6a9956");
static const QTextCharFormat kLanguageKeywordFormat1 =
    TextColorToFormat("#569cd6");
static const QTextCharFormat kLanguageKeywordFormat2 =
    TextColorToFormat("#c586c0");
static const Regex kSqlKeywords = KeywordsRegExp(
    {
        "ABORT",
        "ALL",
        "ALLOCATE",
        "ANALYSE",
        "ANALYZE",
        "AND",
        "ANY",
        "AS",
        "ASC",
        "BETWEEN",
        "BINARY",
        "BIT",
        "BOTH",
        "BY",
        "CASE",
        "CAST",
        "CHAR",
        "CHARACTER",
        "CHECK",
        "CLUSTER",
        "COALESCE",
        "COLLATE",
        "COLLATION",
        "COLUMN",
        "CONSTRAINT",
        "COPY",
        "CREATE",
        "CROSS",
        "CURRENT",
        "CURRENT_CATALOG",
        "CURRENT_DATE",
        "CURRENT_DB",
        "CURRENT_SCHEMA",
        "CURRENT_SID",
        "CURRENT_TIME",
        "CURRENT_TIMESTAMP",
        "CURRENT_USER",
        "CURRENT_USERID",
        "CURRENT_USEROID",
        "DEALLOCATE",
        "DEC",
        "DECIMAL",
        "DECODE",
        "DEFAULT",
        "DELETE",
        "DESC",
        "DISTINCT",
        "DISTRIBUTE",
        "DO",
        "DROP",
        "ELSE",
        "END",
        "EXCEPT",
        "EXCLUDE",
        "EXISTS",
        "EXPLAIN",
        "EXPRESS",
        "EXTEND",
        "EXTERNAL",
        "EXTRACT",
        "FIRST",
        "FLOAT",
        "FOLLOWING",
        "FOR",
        "FOREIGN",
        "FROM",
        "FULL",
        "FUNCTION",
        "GENSTATS",
        "GLOBAL",
        "GROUP",
        "HAVING",
        "IDENTIFIER_CASE",
        "IF",
        "ILIKE",
        "IN",
        "INDEX",
        "INITIALLY",
        "INNER",
        "INOUT",
        "INTERSECT",
        "INTERVAL",
        "INTO",
        "IS",
        "JOIN",
        "LEADING",
        "LEFT",
        "LIKE",
        "LIMIT",
        "LOAD",
        "LOCAL",
        "LOCK",
        "MINUS",
        "MOVE",
        "NATURAL",
        "NCHAR",
        "NEW",
        "NOT",
        "NOTNULL",
        "NULL",
        "NULLS",
        "NUMERIC",
        "NVL",
        "NVL2",
        "OFF",
        "OFFSET",
        "OLD",
        "ON",
        "ONLINE",
        "ONLY",
        "OR",
        "ORDER",
        "OTHERS",
        "OUT",
        "OUTER",
        "OVER",
        "OVERLAPS",
        "PARTITION",
        "POSITION",
        "PRECEDING",
        "PRECISION",
        "PRESERVE",
        "PRIMARY",
        "RESET",
        "REUSE",
        "RIGHT",
        "ROWS",
        "SELECT",
        "SESSION_USER",
        "SETOF",
        "SHOW",
        "SOME",
        "TABLE",
        "THEN",
        "TIES",
        "TIME",
        "TIMESTAMP",
        "TO",
        "TRAILING",
        "TRANSACTION",
        "TRIGGER",
        "TRIM",
        "UNBOUNDED",
        "UNION",
        "UNIQUE",
        "USER",
        "USING",
        "VACUUM",
        "VARCHAR",
        "VERBOSE",
        "VERSION",
        "VIEW",
        "WHEN",
        "WHERE",
        "WITH",
        "WRITE",
        "RESET",
        "REUSE",
    },
    Regex::CaseInsensitiveOption);
static const Regex kQmlKeywords = KeywordsRegExp({
    "import",
    "true",
    "false",
    "property",
    "alias",
    "signal",
    "required",
});
static const Regex kCppKeywords = KeywordsRegExp({
    "alignas",
    "alignof",
    "and",
    "and_eq",
    "asm",
    "atomic_cancel",
    "atomic_commit",
    "atomic_noexcept",
    "auto",
    "bitand",
    "bitor",
    "bool",
    "break",
    "case",
    "catch",
    "char",
    "char8_t",
    "char16_t",
    "char32_t",
    "class",
    "compl",
    "concept",
    "const",
    "consteval",
    "constexpr",
    "constinit",
    "const_cast",
    "continue",
    "co_await",
    "co_return",
    "co_yield",
    "decltype",
    "default",
    "delete",
    "do",
    "double",
    "dynamic_cast",
    "else",
    "enum",
    "explicit",
    "export",
    "extern",
    "false",
    "float",
    "for",
    "friend",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "mutable",
    "namespace",
    "new",
    "noexcept",
    "not",
    "not_eq",
    "nullptr",
    "operator",
    "or",
    "or_eq",
    "private",
    "protected",
    "public",
    "reflexpr",
    "register",
    "reinterpret_cast",
    "requires",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "static_assert",
    "static_cast",
    "struct",
    "switch",
    "synchronized",
    "template",
    "this",
    "thread_local",
    "throw",
    "true",
    "try",
    "typedef",
    "typeid",
    "typename",
    "union",
    "unsigned",
    "using",
    "virtual",
    "void",
    "volatile",
    "wchar_t",
    "while",
    "xor",
    "xor_eq",
});
static const Regex kCPreprocessorKeywords = KeywordsRegExpNoBoundaries({
    "#define",
    "#elif",
    "#else",
    "#endif",
    "#error",
    "#if",
    "#ifdef",
    "#ifndef",
    "#import",
    "#include",
    "#line",
    "#pragma",
    "#undef",
    "#using",
});
static const Regex kJsKeywords = KeywordsRegExp({
    "abstract",     "arguments", "await",    "boolean",    "break",
    "byte",         "case",      "catch",    "char",       "class",
    "const",        "continue",  "debugger", "default",    "delete",
    "do",           "double",    "else",     "enum",       "eval",
    "export",       "extends",   "false",    "final",      "finally",
    "float",        "for",       "function", "goto",       "if",
    "implements",   "import",    "in",       "instanceof", "int",
    "interface",    "let",       "long",     "native",     "new",
    "null",         "package",   "private",  "protected",  "public",
    "return",       "short",     "static",   "super",      "switch",
    "synchronized", "this",      "throw",    "throws",     "transient",
    "true",         "try",       "typeof",   "var",        "void",
    "volatile",     "while",     "with",     "yield",
});

static QList<TextFormat> ParseByRegex(const QString &text,
                                      const QTextCharFormat &format,
                                      const Regex &regex, int end_offset = 0) {
  QList<TextFormat> results;
  for (auto it = regex.globalMatch(text); it.hasNext();) {
    QRegularExpressionMatch m = it.next();
    TextFormat f;
    f.offset = m.capturedStart(0);
    f.length = m.capturedLength(0) + end_offset;
    f.format = format;
    results.append(f);
  }
  return results;
}

static QList<TextFormat> ParseNumbers(const QString &text) {
  static const Regex kRegex("\\b(\\d+(?:\\.\\d+)?|0x[0-9a-fA-F]+)\\b");
  static const QTextCharFormat format = TextColorToFormat("#b5cea8");
  return ParseByRegex(text, format, kRegex);
}

static QList<TextFormat> ParseStrings(const QString &text) {
  static const Regex kRegex("(\".*?(?<!\\\\)\"|'.*?(?<!\\\\)')");
  static const QTextCharFormat format = TextColorToFormat("#ce9178");
  return ParseByRegex(text, format, kRegex);
}

static QList<TextFormat> ParseComments(const QString &text,
                                       const QString &comment_symbol,
                                       const QList<TextFormat> &strings) {
  QList<TextFormat> results;
  int pos = 0;
  while (true) {
    int i = text.indexOf(comment_symbol, pos);
    if (i < 0) {
      break;
    }
    pos = i + comment_symbol.size();
    if (!Syntax::SectionsContain(strings, i)) {
      TextFormat tsf;
      tsf.offset = i;
      i = text.indexOf('\n', pos);
      tsf.length = (i < 0 ? text.size() : i + 1) - tsf.offset;
      tsf.format = kCommentFormat;
      results.append(tsf);
      if (i < 0) {
        break;
      }
      pos = i + 1;
    }
  }
  return results;
}

static QList<TextFormat> ParseStringsAndComments(
    const QString &text, const QString &comment_symbol) {
  QList<TextFormat> results;
  QList<TextFormat> strings = ParseStrings(text);
  results.append(strings);
  results.append(ParseComments(text, comment_symbol, strings));
  return results;
}

static QList<TextFormat> ParseCmake(const QString &text) {
  static const Regex kFunctionRegex("\\b[a-zA-Z0-9\\_]+\\s?\\(");
  static const Regex kCommentRegex("#.*$");
  QList<TextFormat> results;
  results.append(ParseByRegex(text, kFunctionNameFormat, kFunctionRegex, -1));
  results.append(ParseByRegex(text, kCommentFormat, kCommentRegex));
  return results;
}

static QList<TextFormat> ParseSql(const QString &text) {
  static const Regex kBoolRegex =
      KeywordsRegExp({"TRUE", "FALSE"}, Regex::CaseInsensitiveOption);
  QList<TextFormat> results;
  results.append(ParseByRegex(text, kLanguageKeywordFormat1, kSqlKeywords));
  results.append(ParseByRegex(text, kLanguageKeywordFormat2, kBoolRegex));
  results.append(ParseNumbers(text));
  results.append(ParseStringsAndComments(text, "--"));
  return results;
}

static QList<TextFormat> ParseQml(const QString &text) {
  static const Regex kComponentRegex("[a-zA-Z0-9.]+\\s{");
  static const Regex kPropertyRegex("^\\s*[a-zA-Z0-9.]+\\s*:");
  QList<TextFormat> results;
  results.append(ParseByRegex(text, kFunctionNameFormat, kComponentRegex, -1));
  results.append(
      ParseByRegex(text, kLanguageKeywordFormat1, kPropertyRegex, -1));
  results.append(ParseByRegex(text, kLanguageKeywordFormat1, kJsKeywords));
  results.append(ParseByRegex(text, kLanguageKeywordFormat2, kQmlKeywords));
  results.append(ParseNumbers(text));
  results.append(ParseStringsAndComments(text, "//"));
  return results;
}

static QList<TextFormat> ParseCpp(const QString &text) {
  QList<TextFormat> results;
  results.append(ParseByRegex(text, kLanguageKeywordFormat1, kCppKeywords));
  results.append(ParseNumbers(text));
  results.append(
      ParseByRegex(text, kLanguageKeywordFormat2, kCPreprocessorKeywords));
  results.append(ParseStringsAndComments(text, "//"));
  return results;
}

static QList<TextFormat> ParseJs(const QString &text) {
  QList<TextFormat> results;
  results.append(ParseByRegex(text, kLanguageKeywordFormat1, kJsKeywords));
  results.append(ParseNumbers(text));
  results.append(ParseStringsAndComments(text, "//"));
  return results;
}

OldSyntaxFormatter::OldSyntaxFormatter(QObject *parent)
    : TextAreaFormatter(parent), formatter(this) {}

QList<TextSectionFormat> OldSyntaxFormatter::Format(const QString &text,
                                                    const QTextBlock &block) {
  LineInfo line{block.position(), block.firstLineNumber()};
  QList<TextFormat> fs = formatter.Format(text, line);
  QList<TextSectionFormat> result;
  for (const TextFormat &f : fs) {
    TextSectionFormat tsf;
    tsf.section.start = f.offset;
    tsf.section.end = f.offset + f.length - 1;
    tsf.format = f.format;
    result.append(tsf);
  }
  return result;
}

void OldSyntaxFormatter::DetectLanguageByFile(const QString &file_name) {
  formatter.DetectLanguageByFile(file_name);
}

QList<TextFormat> Syntax::FindStringsAndComments(
    const QString &text, const QString &comment_symbol) {
  return ParseStringsAndComments(text, comment_symbol);
}

bool Syntax::SectionsContain(const QList<TextFormat> &sections, int i) {
  for (const TextFormat &sec : sections) {
    if (sec.offset < i && sec.offset + sec.length > i) {
      return true;
    }
  }
  return false;
}

SyntaxFormatter::SyntaxFormatter(QObject *parent) : TextFormatter(parent) {}

QList<TextFormat> SyntaxFormatter::Format(const QString &text, LineInfo) const {
  if (format) {
    return format(text);
  } else {
    return {};
  }
}

void SyntaxFormatter::DetectLanguageByFile(const QString &file_name) {
  if (file_name.endsWith("CMakeLists.txt") || file_name.endsWith(".cmake")) {
    format = ParseCmake;
  } else if (file_name.endsWith(".sql")) {
    format = ParseSql;
  } else if (file_name.endsWith(".qml")) {
    format = ParseQml;
  } else if (file_name.endsWith(".h") || file_name.endsWith(".hxx") ||
             file_name.endsWith(".hpp") || file_name.endsWith(".c") ||
             file_name.endsWith(".cc") || file_name.endsWith(".cxx") ||
             file_name.endsWith(".cpp")) {
    format = ParseCpp;
  } else if (file_name.endsWith(".js")) {
    format = ParseJs;
  } else {
    format = nullptr;
  }
}
