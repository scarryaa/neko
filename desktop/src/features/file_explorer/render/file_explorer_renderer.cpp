#include "features/file_explorer/render/file_explorer_renderer.h"
#include "features/file_explorer/render/types/types.h"
#include "utils/ui_utils.h"
#include <QApplication>
#include <QIcon>
#include <QPainter>
#include <QStyle>

IconInfo FileExplorerRenderer::getIconInfo(FileExplorerRenderState &state,
                                           FileExplorerViewportContext &ctx,
                                           const neko::FileNodeSnapshot &node) {
  // Get the appropriate icon.
  QIcon icon;

  if (node.is_dir) {
    icon = QApplication::style()->standardIcon(
        node.is_expanded ? QStyle::SP_DirOpenIcon : QStyle::SP_DirIcon);
  } else {
    icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
  }

  int iconSize = static_cast<int>(ctx.lineHeight -
                                  FileExplorerRenderConstants::iconAdjustment);
  QIcon colorizedIcon = UiUtils::createColorizedIcon(
      icon, state.theme.selectionColor, QSize(iconSize, iconSize));
  QIcon normalIcon = UiUtils::createColorizedIcon(
      icon, state.theme.fileForegroundColor, QSize(iconSize, iconSize));

  QPixmap pixmap = colorizedIcon.pixmap(iconSize, iconSize);

  if (node.is_hidden) {
    pixmap = colorizedIcon.pixmap(iconSize, iconSize, QIcon::Mode::Disabled,
                                  QIcon::State::Off);
  }

  if (!node.is_dir) {
    pixmap = normalIcon.pixmap(iconSize, iconSize);
  }

  return {.pixmap = pixmap, .size = iconSize};
}

void FileExplorerRenderer::paint(QPainter &painter,
                                 FileExplorerRenderState &state,
                                 FileExplorerViewportContext &ctx) {
  painter.setFont(state.font);

  drawFiles(painter, state, ctx);
}

void FileExplorerRenderer::drawFiles(QPainter &painter,
                                     FileExplorerRenderState &state,
                                     FileExplorerViewportContext &ctx) {
  for (int i = ctx.firstVisibleLine; i < ctx.lastVisibleLine; i++) {
    drawFile(painter, state, ctx, i);
  }
}

void FileExplorerRenderer::drawFile(QPainter &painter,
                                    FileExplorerRenderState &state,
                                    FileExplorerViewportContext &ctx,
                                    int index) {
  double xPosition =
      -ctx.horizontalOffset + FileExplorerRenderConstants::iconEdgePadding;
  double yPosition = (index * ctx.lineHeight) - ctx.verticalOffset;

  auto node = state.snapshot.nodes.at(index);
  double indent =
      static_cast<int>(node.depth) * FileExplorerRenderConstants::nodeIndent;

  // Set up colors.
  auto accentColor = state.theme.selectionColor;
  auto selectionColor = QColor(accentColor);
  auto hoverColor = QColor(state.theme.selectionColor);
  auto fileTextColor = state.theme.fileForegroundColor;

  selectionColor.setAlpha(FileExplorerRenderConstants::selectionAlpha);
  hoverColor.setAlpha(FileExplorerRenderConstants::hoverAlpha);

  // Draw the selection background.
  if (node.is_selected) {
    painter.setBrush(selectionColor);
    painter.setPen(Qt::NoPen);
    painter.drawRect(QRectF(-ctx.horizontalOffset, yPosition,
                            ctx.width +
                                FileExplorerRenderConstants::iconEdgePadding +
                                ctx.horizontalOffset,
                            ctx.lineHeight));
  }

  // Draw the hover background. Only drawn if the hovered node is not selected.
  if (state.hoveredNodePath == QString::fromUtf8(node.path) &&
      !node.is_selected) {
    painter.setBrush(hoverColor);
    painter.setPen(Qt::NoPen);
    painter.drawRect(QRectF(-ctx.horizontalOffset, yPosition,
                            ctx.width +
                                FileExplorerRenderConstants::iconEdgePadding +
                                ctx.horizontalOffset,
                            ctx.lineHeight));
  }

  // Draw the current item border (if applicable).
  if (node.is_current && state.hasFocus) {
    painter.setBrush(Qt::NoBrush);
    painter.setPen(accentColor);
    painter.drawRect(QRectF(-ctx.horizontalOffset, yPosition,
                            ctx.width - 1 + ctx.horizontalOffset,
                            ctx.lineHeight - 1));
  }

  auto iconInfo = getIconInfo(state, ctx, node);

  // Draw the appropriate icon for this node.
  double iconX = xPosition + indent + 2;
  double iconY = yPosition + ((ctx.lineHeight - iconInfo.size) / 2);
  painter.drawPixmap(QPointF(iconX, iconY), iconInfo.pixmap);

  // Draw the node file name.
  double textX = iconX + iconInfo.size + 4;

  if (node.is_hidden) {
    QString foregroundVeryMuted = state.theme.fileHiddenColor;

    painter.setBrush(foregroundVeryMuted);
    painter.setPen(foregroundVeryMuted);
  } else {
    painter.setBrush(fileTextColor);
    painter.setPen(fileTextColor);
  }

  painter.drawText(QPointF(textX, yPosition + state.fontAscent),
                   QString::fromUtf8(node.name));
}

void FileExplorerRenderer::drawDragGhost(QPainter &painter,
                                         FileExplorerRenderState &state,
                                         FileExplorerViewportContext &ctx,
                                         int index) {
  painter.setFont(state.font);

  const auto node = state.snapshot.nodes.at(index);
  double nodeFileNameWidth =
      state.measureFileNameWidth(QString::fromUtf8(node.name));

  const double ghostBackgroundRadius = 4.0;
  auto ghostBackgroundColor = QColor(state.theme.ghostBackgroundColor);
  auto fileTextColor = state.theme.fileForegroundColor;

  auto viewport = painter.viewport();

  // Draw the ghost background.
  painter.setBrush(ghostBackgroundColor);
  painter.setPen(Qt::NoPen);
  painter.drawRoundedRect(viewport, ghostBackgroundRadius,
                          ghostBackgroundRadius);

  // Draw the appropriate icon for this node.
  auto iconInfo = getIconInfo(state, ctx, node);
  double iconX = FileExplorerRenderConstants::iconEdgePadding;
  double iconY = (viewport.height() - iconInfo.size) /
                 FileExplorerRenderConstants::heightDivisor;

  painter.drawPixmap(QPointF(iconX, iconY), iconInfo.pixmap);

  // Draw the node file name.
  double textX = iconX + iconInfo.size + 4;

  if (node.is_hidden) {
    QString foregroundVeryMuted = state.theme.fileHiddenColor;

    painter.setBrush(foregroundVeryMuted);
    painter.setPen(foregroundVeryMuted);
  } else {
    painter.setBrush(fileTextColor);
    painter.setPen(fileTextColor);
  }

  painter.drawText(
      QPointF(textX,
              viewport.height() - (state.fontAscent /
                                   FileExplorerRenderConstants::heightDivisor)),
      QString::fromUtf8(node.name));
}
