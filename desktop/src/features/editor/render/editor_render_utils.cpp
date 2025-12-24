#include "editor_render_utils.h"

double getLineTopY(size_t lineIndex, const ViewportContext &ctx) {
  return (static_cast<int>(lineIndex) * ctx.lineHeight) - ctx.verticalOffset;
}

double getLineBottomY(size_t lineIndex, const ViewportContext &ctx) {
  return ((static_cast<int>(lineIndex) + 1) * ctx.lineHeight) -
         ctx.verticalOffset;
}

QRectF getLineRect(size_t lineIndex, double x1Pos, double x2Pos,
                   const ViewportContext &ctx) {
  return {QPointF(x1Pos, getLineTopY(lineIndex, ctx)),
          QPointF(x2Pos, getLineBottomY(lineIndex, ctx))};
}
