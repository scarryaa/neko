#include "editor_renderer.h"

#include <algorithm>

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

  const int maxLine = std::min(ctx.lastVisibleLine,
                               static_cast<int>(state.rawLines.size()) - 1);
  if (maxLine < ctx.firstVisibleLine) {
    return;
  }

  for (int line = ctx.firstVisibleLine; line <= maxLine; line++) {
    QString lineText = QString::fromUtf8(state.rawLines.at(line));

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
  if (startRow >= state.rawLines.size()) {
    return;
  }

  QString text = QString::fromUtf8(state.rawLines.at(startRow));

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

  if (startRow >= state.rawLines.size()) {
    return;
  }

  QString text = QString::fromUtf8(state.rawLines.at(startRow));

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
    if (i >= state.rawLines.size()) {
      continue;
    }
    if (i > ctx.lastVisibleLine || i < ctx.firstVisibleLine) {
      continue;
    }

    QString text = QString::fromUtf8(state.rawLines.at(i));

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

  if (endRow >= state.rawLines.size()) {
    return;
  }

  QString text = QString::fromUtf8(state.rawLines.at(endRow));
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

    if (cursorRow >= static_cast<int>(state.rawLines.size()) ||
        cursorRow < ctx.firstVisibleLine || cursorRow > ctx.lastVisibleLine) {
      continue;
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
      continue;
    }

    QString text = QString::fromUtf8(state.rawLines.at(cursorRow));
    QString textBeforeCursor = text.left(cursorCol);
    qreal cursorX = state.measureWidth(textBeforeCursor);

    if (cursorX < 0 || cursorX > ctx.width + ctx.horizontalOffset) {
      continue;
    }

    painter->setPen(state.theme.accentColor);
    painter->setBrush(Qt::NoBrush);

    double x = cursorX - ctx.horizontalOffset;
    painter->drawLine(QLineF(QPointF(x, getLineTopY(cursorRow, ctx)),
                             QPointF(x, getLineBottomY(cursorRow, ctx))));
  }
}
