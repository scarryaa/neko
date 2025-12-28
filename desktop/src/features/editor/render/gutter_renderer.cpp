#include "gutter_renderer.h"
#include "features/editor/render/editor_render_utils.h"
#include <QPainter>
#include <QPointF>
#include <QString>

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
    const bool cursorIsOnLine =
        state.isEmpty ? true
                      : rowsWithCursor[static_cast<const int>(line)] != 0;

    const double yPos =
        (line * state.lineHeight) +
        (state.lineHeight + state.fontAscent - state.fontDescent) / 2.0 -
        state.verticalOffset;

    const QString lineNum = QString::number(line + 1);
    const double lineNumWidth = state.measureWidth(lineNum);
    const double xPos = ((ctx.width - maxLineWidth - numWidth) / 2.0) +
                        (maxLineWidth - lineNumWidth) - state.horizontalOffset;

    if (cursorIsOnLine || (selectionActive && line >= selectionStartRow &&
                           line <= selectionEndRow)) {
      painter->setPen(state.theme.activeLineTextColor);
    } else {
      painter->setPen(state.theme.textColor);
    }

    painter->drawText(QPointF(xPos, yPos), lineNum);
  };

  painter->setPen(state.theme.textColor);
  painter->setFont(state.font);

  const int maxLineNumber = state.lineCount;
  const double maxLineWidth =
      state.measureWidth(QString::number(maxLineNumber));
  const double numWidth = state.measureWidth(QString::number(9));

  const auto selection = state.selections;
  const int selectionStartRow = selection.start.row;
  const int selectionEndRow = selection.end.row;
  const int selectionStartCol = selection.start.column;
  const int selectionEndCol = selection.end.column;
  const bool selectionActive = selection.active;

  std::vector<uint8_t> rowsWithCursor(state.lineCount, 0);
  for (const auto &cursor : state.cursors) {
    if (cursor.row >= 0 && cursor.row < rowsWithCursor.size()) {
      rowsWithCursor[cursor.row] = 1;
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
  const auto cursors = state.cursors;

  std::vector<int> highlightedLines = std::vector<int>();
  for (const auto cursor : cursors) {
    const int cursorRow = static_cast<int>(cursor.row);
    const int cursorCol = static_cast<int>(cursor.column);

    if (cursorRow < ctx.firstVisibleLine || cursorRow > ctx.lastVisibleLine) {
      return;
    }

    if (std::find(highlightedLines.begin(), highlightedLines.end(),
                  cursorRow) != highlightedLines.end()) {
      continue;
    }

    highlightedLines.push_back(cursorRow);

    // Draw line highlight
    const auto lineHighlightColor = state.theme.highlightColor;
    painter->setPen(Qt::NoPen);
    painter->setBrush(lineHighlightColor);
    painter->drawRect(getLineRect(cursorRow, 0, ctx.width, ctx));
  }
}
