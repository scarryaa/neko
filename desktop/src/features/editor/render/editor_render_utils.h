#ifndef EDITOR_RENDER_UTILS_H
#define EDITOR_RENDER_UTILS_H

#include "features/editor/render/types/types.h"

double getLineTopY(size_t lineIndex, const ViewportContext &ctx);

double getLineBottomY(size_t lineIndex, const ViewportContext &ctx);

QRectF getLineRect(size_t lineIndex, double x1Pos, double x2Pos,
                   const ViewportContext &ctx);

#endif // EDITOR_RENDER_UTILS_H
