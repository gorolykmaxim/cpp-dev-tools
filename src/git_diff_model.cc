#include "git_diff_model.h"

#include <QFont>
#include <QFontMetrics>
#include <QQmlContext>

#include "application.h"

GitDiffModel::GitDiffModel(QObject *parent)
    : QAbstractListModel(parent),
      before_line_number_max_width(0),
      after_line_number_max_width(0),
      before_selected(true),
      formatter(new SyntaxFormatter(this)) {
  connect(this, &GitDiffModel::rawDiffChanged, this, &GitDiffModel::ParseDiff);
  connect(this, &GitDiffModel::fileChanged, this,
          [this] { formatter->DetectLanguageByFile(file); });
}

int GitDiffModel::rowCount(const QModelIndex &) const { return lines.size(); }

QVariant GitDiffModel::data(const QModelIndex &index, int role) const {
  const DiffLine &line = lines[index.row()];
  switch (role) {
    case 0:
      return line.before_line_number;
    case 1:
      return raw_diff.sliced(line.before_line.offset, line.before_line.length);
    case 2:
      return line.after_line_number;
    case 3:
      return raw_diff.sliced(line.after_line.offset, line.after_line.length);
    case 4:
      if (line.header.length == 0) {
        return "";
      } else {
        return raw_diff.sliced(line.header.offset, line.header.length);
      }
    case 5:
      return line.is_delete;
    case 6:
      return line.is_add;
    default:
      return QVariant();
  }
}

QHash<int, QByteArray> GitDiffModel::roleNames() const {
  return {
      {0, "beforeLineNumber"},
      {1, "beforeLine"},
      {2, "afterLineNumber"},
      {3, "afterLine"},
      {4, "header"},
      {5, "isDelete"},
      {6, "isAdd"},
  };
}

static int ParseLineNumber(const QString &str, QChar start) {
  int i = str.indexOf(start) + 1;
  int j = str.indexOf(',', i);
  if (j < 0) {
    j = str.indexOf(' ', i);
  }
  return str.sliced(i, j - i).toInt();
}

static void WriteBeforeAfterBuffers(QList<DiffLine> &before_buff,
                                    QList<DiffLine> &after_buff,
                                    QList<DiffLine> &new_lines,
                                    TextSegment header) {
  int cnt = std::max(before_buff.size(), after_buff.size());
  for (int i = 0; i < cnt; i++) {
    DiffLine dl;
    if (i < before_buff.size()) {
      const DiffLine &before = before_buff[i];
      dl.is_delete = before.is_delete;
      dl.before_line_number = before.before_line_number;
      dl.before_line = before.before_line;
    }
    if (i < after_buff.size()) {
      const DiffLine &after = after_buff[i];
      dl.is_add = after.is_add;
      dl.after_line_number = after.after_line_number;
      dl.after_line = after.after_line;
    }
    dl.header = header;
    new_lines.append(dl);
  }
  before_buff.clear();
  after_buff.clear();
}

void GitDiffModel::ParseDiff() {
  QList<DiffLine> new_lines;
  int pos = 0;
  bool is_header = true;
  TextSegment header;
  int before_line_number = -1, after_line_number = -1;
  QList<DiffLine> before_buff, after_buff;
  while (true) {
    int i = raw_diff.indexOf('\n', pos);
    if (i < 0) {
      break;
    }
    QString line = raw_diff.sliced(pos, i - pos);
    if (line.startsWith("@@")) {
      is_header = false;
      header.offset = pos;
      header.length = line.size();
      before_line_number = ParseLineNumber(line, '-');
      after_line_number = ParseLineNumber(line, '+');
    } else if (is_header) {
    } else if (line.startsWith('-')) {
      DiffLine dl;
      dl.is_delete = true;
      dl.before_line_number = before_line_number++;
      dl.before_line = TextSegment{pos + 1, (int)line.size() - 1};
      dl.header = header;
      before_buff.append(dl);
    } else if (line.startsWith('+')) {
      DiffLine dl;
      dl.is_add = true;
      dl.after_line_number = after_line_number++;
      dl.after_line = TextSegment{pos + 1, (int)line.size() - 1};
      dl.header = header;
      after_buff.append(dl);
    } else if (line.startsWith(' ')) {
      WriteBeforeAfterBuffers(before_buff, after_buff, new_lines, header);
      DiffLine dl;
      dl.header = header;
      dl.before_line_number = before_line_number++;
      dl.before_line = TextSegment{pos + 1, (int)line.size() - 1};
      dl.after_line_number = after_line_number++;
      dl.after_line = dl.before_line;
      new_lines.append(dl);
    }
    pos = i + 1;
  }
  WriteBeforeAfterBuffers(before_buff, after_buff, new_lines, header);
  if (!lines.isEmpty()) {
    beginRemoveRows(QModelIndex(), 0, lines.size() - 1);
    lines.clear();
    endRemoveRows();
  }
  if (!new_lines.isEmpty()) {
    beginInsertRows(QModelIndex(), 0, new_lines.size() - 1);
    lines = new_lines;
    endInsertRows();
  }
  Application &app = Application::Get();
  QQmlContext *ctx = app.qml_engine.rootContext();
  QString family = ctx->contextProperty("monoFontFamily").toString();
  int size = ctx->contextProperty("monoFontSize").toInt();
  QFontMetrics m(QFont(family, size));
  before_line_number_max_width =
      m.horizontalAdvance(QString::number(before_line_number));
  after_line_number_max_width =
      m.horizontalAdvance(QString::number(after_line_number));
  emit modelChanged();
}
