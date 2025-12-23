#include "editor_renderer.h"

void EditorRenderer::paint(QPainter &painter, const RenderState &state,
                           const ViewportContext &ctx) {
  drawText(&painter, state, ctx);
  drawCursors(&painter, state, ctx);
  drawSelections(&painter, state, ctx);
}

void EditorRenderer::drawText(QPainter *painter, const RenderState &state,
                              const ViewportContext &ctx) {
  auto drawLine = [painter, state, ctx](auto line) {
    const QString lineText = state.isEmpty ? "" : state.lines.at(line);

    const auto actualY =
        (line * ctx.lineHeight) +
        (ctx.lineHeight + state.fontAscent - state.fontDescent) / 2.0 -
        ctx.verticalOffset;

    painter->drawText(QPointF(-ctx.horizontalOffset, actualY), lineText);
  };

  painter->setPen(state.theme.textColor);
  painter->setFont(state.font);

  const int maxLine = std::min(ctx.lastVisibleLine,
                               static_cast<const int>(state.lines.size()) - 1);
  if (ctx.firstVisibleLine <= 0 && ctx.lastVisibleLine <= 0) {
    drawLine(0);
    return;
  }

  if (maxLine < ctx.firstVisibleLine) {
    return;
  }

  for (int line = ctx.firstVisibleLine; line <= maxLine; line++) {
    drawLine(line);
  }
}

void EditorRenderer::drawSelections(QPainter *painter, const RenderState &state,
                                    const ViewportContext &ctx) {
  const bool hasActiveSelection = state.selections.active;
  if (!hasActiveSelection) {
    return;
  }

  auto selectionColor = QColor(state.theme.accentColor);
  selectionColor.setAlpha(SELECTION_ALPHA);
  painter->setBrush(selectionColor);
  painter->setPen(Qt::transparent);

  const auto selection = state.selections;
  const int startRow = static_cast<int>(selection.start.row);
  const int endRow = static_cast<int>(selection.end.row);
  const int startCol = static_cast<int>(selection.start.col);
  const int endCol = static_cast<int>(selection.end.col);

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
                                             // NOLINTNEXTLINE
                                             int startRow, int startCol,
                                             const int endCol) {
  if (startRow >= state.lines.size()) {
    return;
  }

  const QString text = state.lines.at(startRow);

  const QString selection_text = text.mid(startCol, endCol - startCol);
  const QString selection_before_text = text.mid(0, startCol);

  const double width = state.measureWidth(selection_text);
  const double widthBefore = state.measureWidth(selection_before_text);

  const double x1Pos = widthBefore - ctx.horizontalOffset;
  const double x2Pos = widthBefore + width - ctx.horizontalOffset;

  painter->drawRect(getLineRect(startRow, x1Pos, x2Pos, ctx));
}

void EditorRenderer::drawFirstLineSelection(QPainter *painter,
                                            const RenderState &state,
                                            const ViewportContext &ctx,
                                            // NOLINTNEXTLINE
                                            int startRow, int startCol) {
  if (startRow > ctx.lastVisibleLine || startRow < ctx.firstVisibleLine) {
    return;
  }

  if (startRow >= state.lines.size()) {
    return;
  }

  QString text = state.lines.at(startRow);

  if (text.isEmpty()) {
    text = " ";
  }

  const QString selectionText = text.mid(startCol);
  const QString beforeText = text.mid(0, startCol);

  const double width = state.measureWidth(selectionText);
  const double widthBefore = state.measureWidth(beforeText);

  const double x1Pos = widthBefore - ctx.horizontalOffset;
  const double x2Pos = widthBefore + width - ctx.horizontalOffset;

  painter->drawRect(getLineRect(startRow, x1Pos, x2Pos, ctx));
}

void EditorRenderer::drawMiddleLinesSelection(QPainter *painter,
                                              const RenderState &state,
                                              const ViewportContext &ctx,
                                              // NOLINTNEXTLINE
                                              int startRow, int endRow) {
  for (int i = startRow + 1; i < endRow; i++) {
    if (i >= state.lines.size()) {
      continue;
    }
    if (i > ctx.lastVisibleLine || i < ctx.firstVisibleLine) {
      continue;
    }

    QString text = state.lines.at(i);

    if (text.isEmpty()) {
      text = " ";
    }

    const double x1Pos = -ctx.horizontalOffset;
    const double x2Pos = state.measureWidth(text) - ctx.horizontalOffset;

    painter->drawRect(getLineRect(i, x1Pos, x2Pos, ctx));
  }
}

void EditorRenderer::drawLastLineSelection(QPainter *painter,
                                           const RenderState &state,
                                           const ViewportContext &ctx,
                                           // NOLINTNEXTLINE
                                           int endRow, int endCol) {
  if (endRow > ctx.lastVisibleLine || endRow < ctx.firstVisibleLine) {
    return;
  }

  if (endRow >= state.lines.size()) {
    return;
  }

  const QString text = state.lines.at(endRow);
  const QString selectionText = text.mid(0, endCol);

  const double width = state.measureWidth(selectionText);

  painter->drawRect(getLineRect(endRow, -ctx.horizontalOffset,
                                width - ctx.horizontalOffset, ctx));
}

void EditorRenderer::drawCursors(QPainter *painter, const RenderState &state,
                                 const ViewportContext &ctx) {
  std::vector<int> highlightedLines;

  const auto drawCursor = [&](int cursorRow, int cursorCol) {
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
    if (!state.isEmpty &&
        (cursorRow < 0 ||
         cursorRow >= static_cast<const int>(state.lines.size()))) {
      return;
    }

    const QString text = state.isEmpty ? "" : state.lines.at(cursorRow);
    const int clampedCol =
        std::clamp(cursorCol, 0, static_cast<const int>(text.size()));

    const qreal cursorX = state.measureWidth(text.left(clampedCol));
    if (cursorX < 0 || cursorX > ctx.width + ctx.horizontalOffset) {
      return;
    }

    painter->setPen(state.theme.accentColor);
    painter->setBrush(Qt::NoBrush);

    const double finalCursorX = cursorX - ctx.horizontalOffset;
    painter->drawLine(
        QLineF(QPointF(finalCursorX, getLineTopY(cursorRow, ctx)),
               QPointF(finalCursorX, getLineBottomY(cursorRow, ctx))));
  };

  if (state.isEmpty) {
    drawCursor(0, 0);
    return;
  }

  for (const auto &cursor : state.cursors) {
    if (cursor.row < ctx.firstVisibleLine || cursor.row > ctx.lastVisibleLine) {
      continue;
    }
    if (cursor.row < 0 ||
        cursor.row >= static_cast<const int>(state.lines.size())) {
      continue;
    }

    drawCursor(static_cast<int>(cursor.row), static_cast<int>(cursor.col));
  }
}
