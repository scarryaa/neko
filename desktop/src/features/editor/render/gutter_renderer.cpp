#include "gutter_renderer.h"

void GutterRenderer::paint(QPainter &painter, const RenderState &state,
                           const ViewportContext &ctx) {
  drawText(&painter, state, ctx);
  drawLineHighlight(&painter, state, ctx);
}

void GutterRenderer::drawText(QPainter *painter, const RenderState &state,
                              const ViewportContext &ctx) {
  auto drawLine = [painter, state,
                   ctx](auto rowsWithCursor, auto line, double maxLineWidth,
                        double numWidth, bool selectionActive,
                        int selectionStartRow, int selectionEndRow) {
    bool cursorIsOnLine =
        state.isEmpty ? true : rowsWithCursor[static_cast<size_t>(line)] != 0;

    auto y = (line * state.lineHeight) +
             (state.lineHeight + state.fontAscent - state.fontDescent) / 2.0 -
             state.verticalOffset;

    QString lineNum = QString::number(line + 1);
    int lineNumWidth = state.measureWidth(lineNum);
    double x = (ctx.width - maxLineWidth - numWidth) / 2.0 +
               (maxLineWidth - lineNumWidth) - state.horizontalOffset;

    if (cursorIsOnLine || (selectionActive && line >= selectionStartRow &&
                           line <= selectionEndRow)) {
      painter->setPen(state.theme.activeLineTextColor);
    } else {
      painter->setPen(state.theme.textColor);
    }

    painter->drawText(QPointF(x, y), lineNum);
  };

  painter->setPen(state.theme.textColor);
  painter->setFont(state.font);

  int maxLineNumber = state.lineCount;
  int maxLineWidth = state.measureWidth(QString::number(maxLineNumber));
  double numWidth = state.measureWidth(QString::number(9));

  neko::Selection selection = state.selections;
  int selectionStartRow = selection.start.row;
  int selectionEndRow = selection.end.row;
  int selectionStartCol = selection.start.col;
  int selectionEndCol = selection.end.col;
  bool selectionActive = selection.active;

  std::vector<uint8_t> rowsWithCursor(state.lineCount, 0);
  for (const auto &c : state.cursors) {
    if (c.row >= 0 && static_cast<size_t>(c.row) < rowsWithCursor.size()) {
      rowsWithCursor[static_cast<size_t>(c.row)] = 1;
    }
  }

  if (ctx.firstVisibleLine <= 0 && ctx.lastVisibleLine <= 0) {
    drawLine(rowsWithCursor, 0, maxLineWidth, numWidth, selectionActive,
             selectionStartRow, selectionEndRow);
    return;
  }

  for (int line = ctx.firstVisibleLine; line <= ctx.lastVisibleLine; ++line) {
    drawLine(rowsWithCursor, line, maxLineWidth, numWidth, selectionActive,
             selectionStartRow, selectionEndRow);
  }
}

void GutterRenderer::drawLineHighlight(QPainter *painter,
                                       const RenderState &state,
                                       const ViewportContext &ctx) {
  auto cursors = state.cursors;

  std::vector<int> highlightedLines = std::vector<int>();
  for (auto cursor : cursors) {
    int cursorRow = cursor.row;
    int cursorCol = cursor.col;

    if (cursorRow < ctx.firstVisibleLine || cursorRow > ctx.lastVisibleLine) {
      return;
    }

    if (std::find(highlightedLines.begin(), highlightedLines.end(),
                  cursorRow) != highlightedLines.end()) {
      continue;
    }

    highlightedLines.push_back(cursorRow);

    // Draw line highlight
    auto lineHighlightColor = state.theme.highlightColor;
    painter->setPen(Qt::NoPen);
    painter->setBrush(lineHighlightColor);
    painter->drawRect(getLineRect(cursorRow, 0, ctx.width, ctx));
  }
}
