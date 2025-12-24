#ifndef GUTTER_RENDERER_H
#define GUTTER_RENDERER_H

#include "editor_render_utils.h"
#include "render_state.h"
#include <QPainter>

class GutterRenderer {
public:
  static void paint(QPainter &painter, const RenderState &state,
                    const ViewportContext &ctx);

private:
  static void drawText(QPainter *painter, const RenderState &state,
                       const ViewportContext &ctx);
  static void drawLineHighlight(QPainter *painter, const RenderState &state,
                                const ViewportContext &ctx);
};

#endif // GUTTER_RENDERER_H
