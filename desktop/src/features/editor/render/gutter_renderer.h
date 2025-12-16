#ifndef GUTTER_RENDERER_H
#define GUTTER_RENDERER_H

#include "render_state.h"
#include "utils/editor_utils.h"
#include <QPainter>

class GutterRenderer {
public:
  void paint(QPainter &painter, const RenderState &state,
             const ViewportContext &ctx);

private:
  void drawText(QPainter *painter, const RenderState &state,
                const ViewportContext &ctx);
  void drawLineHighlight(QPainter *painter, const RenderState &state,
                         const ViewportContext &ctx);
};

#endif // GUTTER_RENDERER_H
