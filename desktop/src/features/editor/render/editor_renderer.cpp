#include "editor_renderer.h"

void EditorRenderer::paint(QPainter &painter, const RenderState &state,
                           const ViewportContext &ctx) {
  drawText(&painter, state, ctx);
  drawCursors(&painter, state, ctx);
  drawSelections(&painter, state, ctx);
}

void EditorRenderer::drawText(QPainter *painter, const RenderState &state,
                              const ViewportContext &ctx) {
  painter->setPen(state.theme.textColor);
  painter->setFont(state.font);

  for (int line = ctx.firstVisibleLine; line <= ctx.lastVisibleLine; line++) {
    auto rawLine = state.rawLines.at(line);
    QString lineText = QString::fromUtf8(rawLine);

    auto actualY =
        (line * ctx.lineHeight) +
        (ctx.lineHeight + state.fontAscent - state.fontDescent) / 2.0 -
        ctx.verticalOffset;

    painter->drawText(QPointF(-ctx.horizontalOffset, actualY), lineText);
  }
}

void EditorRenderer::drawSelections(QPainter *painter, const RenderState &state,
                                    const ViewportContext &ctx) {
  bool hasActiveSelection = state.selections.active;
  if (!hasActiveSelection) {
    return;
  }

  QColor selectionColor = QColor(state.theme.accentColor);
  selectionColor.setAlpha(50);
  painter->setBrush(selectionColor);
  painter->setPen(Qt::transparent);

  auto selection = state.selections;
  int startRow = selection.start.row;
  int endRow = selection.end.row;
  int startCol = selection.start.col;
  int endCol = selection.end.col;

  if (startRow == endRow) {
    drawSingleLineSelection(painter, state, ctx, startRow, startCol, endCol);
  } else {
    drawFirstLineSelection(painter, state, ctx, startRow, startCol);
    drawMiddleLinesSelection(painter, state, ctx, startRow, endRow);
    drawLastLineSelection(painter, state, ctx, endRow, endCol);
  }
}

void EditorRenderer::drawSingleLineSelection(QPainter *painter,
                                             const RenderState &state,
                                             const ViewportContext &ctx,
                                             size_t startRow, size_t startCol,
                                             size_t endCol) {
  auto rawLine = state.rawLines.at(startRow);
  QString text = QString::fromUtf8(rawLine);

  QString selection_text = text.mid(startCol, endCol - startCol);
  QString selection_before_text = text.mid(0, startCol);

  double width = state.measureWidth(selection_text);
  double widthBefore = state.measureWidth(selection_before_text);

  double x1 = widthBefore - ctx.horizontalOffset;
  double x2 = widthBefore + width - ctx.horizontalOffset;

  painter->drawRect(getLineRect(startRow, x1, x2, ctx));
}

void EditorRenderer::drawFirstLineSelection(QPainter *painter,
                                            const RenderState &state,
                                            const ViewportContext &ctx,
                                            size_t startRow, size_t startCol) {
  if (startRow > ctx.lastVisibleLine || startRow < ctx.firstVisibleLine) {
    return;
  }

  auto rawLine = state.rawLines.at(startRow);
  QString text = QString::fromUtf8(rawLine);

  if (text.isEmpty())
    text = " ";

  QString selectionText = text.mid(startCol);
  QString beforeText = text.mid(0, startCol);

  double width = state.measureWidth(selectionText);
  double widthBefore = state.measureWidth(beforeText);

  double x1 = widthBefore - ctx.horizontalOffset;
  double x2 = widthBefore + width - ctx.horizontalOffset;

  painter->drawRect(getLineRect(startRow, x1, x2, ctx));
}

void EditorRenderer::drawMiddleLinesSelection(QPainter *painter,
                                              const RenderState &state,
                                              const ViewportContext &ctx,
                                              size_t startRow, size_t endRow) {
  for (size_t i = startRow + 1; i < endRow; i++) {
    if (i > ctx.lastVisibleLine || i < ctx.firstVisibleLine) {
      continue;
    }

    auto rawLine = state.rawLines.at(i);
    QString text = QString::fromUtf8(rawLine);

    if (text.isEmpty())
      text = " ";

    double x1 = -ctx.horizontalOffset;
    double x2 = state.measureWidth(text) - ctx.horizontalOffset;

    painter->drawRect(getLineRect(i, x1, x2, ctx));
  }
}

void EditorRenderer::drawLastLineSelection(QPainter *painter,
                                           const RenderState &state,
                                           const ViewportContext &ctx,
                                           size_t endRow, size_t endCol) {
  if (endRow > ctx.lastVisibleLine || endRow < ctx.firstVisibleLine) {
    return;
  }

  auto rawLine = state.rawLines.at(endRow);
  QString text = QString::fromUtf8(rawLine);
  QString selectionText = text.mid(0, endCol);

  double width = state.measureWidth(selectionText);

  painter->drawRect(getLineRect(endRow, -ctx.horizontalOffset,
                                width - ctx.horizontalOffset, ctx));
}

void EditorRenderer::drawCursors(QPainter *painter, const RenderState &state,
                                 const ViewportContext &ctx) {
  auto cursors = state.cursors;

  auto highlightedLines = std::vector<int>();
  for (auto cursor : cursors) {
    int cursorRow = cursor.row;
    int cursorCol = cursor.col;

    if (cursorRow < ctx.firstVisibleLine || cursorRow > ctx.lastVisibleLine) {
      return;
    }

    // Draw line highlight
    if (std::find(highlightedLines.begin(), highlightedLines.end(),
                  cursorRow) == highlightedLines.end()) {
      painter->setPen(Qt::NoPen);
      painter->setBrush(state.theme.highlightColor);
      painter->drawRect(
          getLineRect(cursorRow, 0, ctx.width + ctx.horizontalOffset, ctx));

      highlightedLines.push_back(cursorRow);
    }

    if (!state.hasFocus) {
      return;
    }

    auto rawLine = state.rawLines.at(cursorRow);
    QString text = QString::fromUtf8(rawLine);
    QString textBeforeCursor = text.left(cursorCol);
    qreal cursorX = state.measureWidth(textBeforeCursor);

    if (cursorX < 0 || cursorX > ctx.width + ctx.horizontalOffset) {
      return;
    }

    painter->setPen(state.theme.accentColor);
    painter->setBrush(Qt::NoBrush);

    double x = cursorX - ctx.horizontalOffset;
    painter->drawLine(QLineF(QPointF(x, getLineTopY(cursorRow, ctx)),
                             QPointF(x, getLineBottomY(cursorRow, ctx))));
  }
}
