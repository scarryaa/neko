#ifndef EDITOR_UTILS_H
#define EDITOR_UTILS_H

#include <QRectF>

struct ViewportContext {
  double lineHeight;
  int firstVisibleLine;
  int lastVisibleLine;
  double verticalOffset;
  double horizontalOffset;
};

double getLineTopY(size_t lineIndex, const ViewportContext &ctx);

double getLineBottomY(size_t lineIndex, const ViewportContext &ctx);

QRectF getLineRect(size_t lineIndex, double x1, double x2,
                   const ViewportContext &ctx);

#endif // EDITOR_UTILS_H
