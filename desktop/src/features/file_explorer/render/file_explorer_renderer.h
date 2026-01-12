#ifndef FILE_EXPLORER_RENDERER_H
#define FILE_EXPLORER_RENDERER_H

#include "features/file_explorer/render/types/types.h"
#include "types/qt_types_fwd.h"
#include <QPixmap>

QT_FWD(QPainter)

class FileExplorerRenderer {
public:
  static IconInfo getIconInfo(FileExplorerRenderState &state,
                              FileExplorerViewportContext &ctx,
                              const neko::FileNodeSnapshot &node);

  static void paint(QPainter &painter, FileExplorerRenderState &state,
                    FileExplorerViewportContext &ctx);
  static void drawDragGhost(QPainter &painter, FileExplorerRenderState &state,
                            FileExplorerViewportContext &ctx, int index);

private:
  static void drawFiles(QPainter &painter, FileExplorerRenderState &state,
                        FileExplorerViewportContext &ctx);
  static void drawFile(QPainter &painter, FileExplorerRenderState &state,
                       FileExplorerViewportContext &ctx, int index);
};

#endif
