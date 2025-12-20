#ifndef EDITOR_RENDERER_H
#define EDITOR_RENDERER_H

#include "render_state.h"
#include "utils/editor_utils.h"
#include <QPainter>
#include <neko-core/src/ffi/mod.rs.h>

class EditorRenderer {
public:
  void paint(QPainter &painter, const RenderState &state,
             const ViewportContext &ctx) const;

private:
  void drawText(QPainter *painter, const RenderState &state,
                const ViewportContext &ctx) const;
  void drawCursors(QPainter *painter, const RenderState &state,
                   const ViewportContext &ctx) const;
  void drawSelections(QPainter *painter, const RenderState &state,
                      const ViewportContext &ctx) const;
  void drawSingleLineSelection(QPainter *painter, const RenderState &state,
                               const ViewportContext &ctx, const int startRow,
                               const int startCol, const int endCol) const;
  void drawFirstLineSelection(QPainter *painter, const RenderState &state,
                              const ViewportContext &ctx, const int startRow,
                              const int startCol) const;
  void drawMiddleLinesSelection(QPainter *painter, const RenderState &state,
                                const ViewportContext &ctx, const int startRow,
                                const int endRow) const;
  void drawLastLineSelection(QPainter *painter, const RenderState &state,
                             const ViewportContext &ctx, const int endRow,
                             const int endCol) const;
};

#endif
