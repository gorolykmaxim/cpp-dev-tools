#include "git_diff_model.h"

#include <QClipboard>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QQmlContext>

#include "application.h"

GitDiffModel::GitDiffModel(QObject *parent)
    : QAbstractListModel(parent),
      before_line_number_max_width(0),
      after_line_number_max_width(0),
      selected_side(0),
      syntax_formatter(new SyntaxFormatter(this)),
      selection_formatter(new SelectionFormatter(this, selection)) {
  connect(this, &GitDiffModel::rawDiffChanged, this, &GitDiffModel::ParseDiff);
  connect(this, &GitDiffModel::fileChanged, this,
          [this] { syntax_formatter->DetectLanguageByFile(file); });
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

void GitDiffModel::SetSelectedSide(int side) {
  if (side == selected_side) {
    return;
  }
  std::pair<int, int> redraw_range = selection.GetLineRange();
  selection = TextSelection{};
  emit rehighlightLines(redraw_range.first, redraw_range.second);
  selected_side = side;
  emit selectedSideChanged();
}

int GitDiffModel::GetSelectedSide() const { return selected_side; }

bool GitDiffModel::resetSelection() {
  if (selection.first_line < 0) {
    return false;
  }
  std::pair<int, int> redraw_range = selection.GetLineRange();
  selection = TextSelection();
  emit rehighlightLines(redraw_range.first, redraw_range.second);
  return true;
}

void GitDiffModel::copySelection(int current_line) {
  TextSelection s = selection.Normalize();
  if (s.first_line < 0) {
    s.first_line = s.last_line = current_line;
  }
  QStringList lines_to_copy;
  for (int i = s.first_line; i <= s.last_line; i++) {
    const DiffLine &dl = lines[i];
    if ((selected_side == 0 && dl.before_line_number < 0) ||
        (selected_side == 1 && dl.after_line_number < 0)) {
      continue;
    }
    TextSegment segment = selected_side == 0 ? dl.before_line : dl.after_line;
    if (i == s.last_line && s.last_line_offset >= 0) {
      segment.length = s.last_line_offset;
    }
    if (i == s.first_line && s.first_line_offset >= 0) {
      segment.offset += s.first_line_offset;
      segment.length -= s.first_line_offset;
    }
    QString text = raw_diff.sliced(segment.offset, segment.length);
    lines_to_copy.append(text);
  }
  QGuiApplication::clipboard()->setText(lines_to_copy.join('\n'));
}

void GitDiffModel::selectInline(int line, int start, int end) {
  std::pair<int, int> redraw_range = selection.GetLineRange();
  selection.multiline_selection = false;
  selection.last_line = selection.first_line = line;
  selection.first_line_offset = start;
  selection.last_line_offset = end;
  emit rehighlightLines(redraw_range.first, redraw_range.second);
  emit rehighlightLines(line, line);
}

void GitDiffModel::selectLine(int line) {
  if (line < 0 || line >= lines.size()) {
    return;
  }
  if (!selection.multiline_selection) {
    std::pair<int, int> redraw_range = selection.GetLineRange();
    selection.multiline_selection = true;
    selection.first_line = selection.last_line = line;
    selection.first_line_offset = selection.last_line_offset = -1;
    emit rehighlightLines(redraw_range.first, redraw_range.second);
    emit rehighlightLines(line, line);
  } else {
    std::pair<int, int> redraw_range = selection.GetLineRange();
    selection.last_line = line;
    emit rehighlightLines(redraw_range.first, redraw_range.second);
    redraw_range = selection.GetLineRange();
    emit rehighlightLines(redraw_range.first, redraw_range.second);
  }
}

void GitDiffModel::selectAll() {
  std::pair<int, int> redraw_range = selection.GetLineRange();
  selection = TextSelection();
  selection.first_line = 0;
  selection.last_line = std::max((int)lines.size() - 1, 0);
  emit rehighlightLines(redraw_range.first, redraw_range.second);
  redraw_range = selection.GetLineRange();
  emit rehighlightLines(redraw_range.first, redraw_range.second);
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
