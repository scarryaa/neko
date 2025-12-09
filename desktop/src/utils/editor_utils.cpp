#include "editor_utils.h"

double getLineTopY(size_t lineIndex, const ViewportContext &ctx) {
  return (lineIndex * ctx.lineHeight) - ctx.verticalOffset;
}

double getLineBottomY(size_t lineIndex, const ViewportContext &ctx) {
  return ((lineIndex + 1) * ctx.lineHeight) - ctx.verticalOffset;
}

QRectF getLineRect(size_t lineIndex, double x1, double x2,
                   const ViewportContext &ctx) {
  return QRectF(QPointF(x1, getLineTopY(lineIndex, ctx)),
                QPointF(x2, getLineBottomY(lineIndex, ctx)));
}
