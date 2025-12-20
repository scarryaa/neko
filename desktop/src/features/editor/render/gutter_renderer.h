#ifndef GUTTER_RENDERER_H
#define GUTTER_RENDERER_H

#include "render_state.h"
#include "utils/editor_utils.h"
#include <QPainter>

class GutterRenderer {
public:
  void paint(QPainter &painter, const RenderState &state,
             const ViewportContext &ctx) const;

private:
  void drawText(QPainter *painter, const RenderState &state,
                const ViewportContext &ctx) const;
  void drawLineHighlight(QPainter *painter, const RenderState &state,
                         const ViewportContext &ctx) const;
};

#endif // GUTTER_RENDERER_H
