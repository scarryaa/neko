#ifndef EDITOR_RENDERER_H
#define EDITOR_RENDERER_H

#include "features/editor/render/types/types.h"
#include "types/qt_types_fwd.h"

QT_FWD(QPainter)

class EditorRenderer {
public:
  static void paint(QPainter &painter, const RenderState &state,
                    const ViewportContext &ctx);

private:
  static void drawText(QPainter *painter, const RenderState &state,
                       const ViewportContext &ctx);
  static void drawCursors(QPainter *painter, const RenderState &state,
                          const ViewportContext &ctx);
  static void drawSelections(QPainter *painter, const RenderState &state,
                             const ViewportContext &ctx);
  static void drawSingleLineSelection(QPainter *painter,
                                      const RenderState &state,
                                      const ViewportContext &ctx, int startRow,
                                      int startCol, int endCol);
  static void drawFirstLineSelection(QPainter *painter,
                                     const RenderState &state,
                                     const ViewportContext &ctx, int startRow,
                                     int startCol);
  static void drawMiddleLinesSelection(QPainter *painter,
                                       const RenderState &state,
                                       const ViewportContext &ctx, int startRow,
                                       int endRow);
  static void drawLastLineSelection(QPainter *painter, const RenderState &state,
                                    const ViewportContext &ctx, int endRow,
                                    int endCol);

  static constexpr double SELECTION_ALPHA = 50.0;
};

#endif
