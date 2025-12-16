#ifndef EDITOR_RENDERER_H
#define EDITOR_RENDERER_H

#include "render_state.h"
#include "utils/editor_utils.h"
#include <QPainter>
#include <neko-core/src/ffi/mod.rs.h>

class EditorRenderer {
public:
  void paint(QPainter &painter, const RenderState &state,
             const ViewportContext &ctx);

private:
  void drawText(QPainter *painter, const RenderState &state,
                const ViewportContext &ctx);
  void drawCursors(QPainter *painter, const RenderState &state,
                   const ViewportContext &ctx);
  void drawSelections(QPainter *painter, const RenderState &state,
                      const ViewportContext &ctx);
  void drawSingleLineSelection(QPainter *painter, const RenderState &state,
                               const ViewportContext &ctx, size_t startRow,
                               size_t startCol, size_t endCol);
  void drawFirstLineSelection(QPainter *painter, const RenderState &state,
                              const ViewportContext &ctx, size_t startRow,
                              size_t startCol);
  void drawMiddleLinesSelection(QPainter *painter, const RenderState &state,
                                const ViewportContext &ctx, size_t startRow,
                                size_t endRow);
  void drawLastLineSelection(QPainter *painter, const RenderState &state,
                             const ViewportContext &ctx, size_t endRow,
                             size_t endCol);
};

#endif
