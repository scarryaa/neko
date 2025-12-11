#ifndef EDITOR_WIDGET_H
#define EDITOR_WIDGET_H

#include "utils/editor_utils.h"
#include "utils/gui_utils.h"
#include "utils/row_col.h"
#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QtDebug>
#include <neko_core.h>

class EditorWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit EditorWidget(NekoEditor *editor, NekoConfigManager *configManager,
                        NekoThemeManager *themeManager,
                        QWidget *parent = nullptr);
  ~EditorWidget();

  void updateDimensionsAndRepaint();
  void setEditor(NekoEditor *editor);

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  bool focusNextPrevChild(bool next) override;

signals:
  void fontSizeChanged(qreal newSize);
  void lineCountChanged();
  void cursorPositionChanged();
  void newTabRequested();
  void closeTabRequested();
  void bufferChanged();

private:
  double getTextWidth(const QString &text, double horizontalOffset) const;
  RowCol convertMousePositionToRowCol(double x, double y);

  void drawText(QPainter *painter, const ViewportContext &ctx);
  void drawCursor(QPainter *painter, const ViewportContext &ctx);
  void drawSelections(QPainter *painter, const ViewportContext &ctx);

  void drawSingleLineSelection(QPainter *painter, const ViewportContext &ctx,
                               size_t startRow, size_t startCol, size_t endCol);
  void drawFirstLineSelection(QPainter *painter, const ViewportContext &ctx,
                              size_t startRow, size_t startCol);
  void drawMiddleLinesSelection(QPainter *painter, const ViewportContext &ctx,
                                size_t startRow, size_t endRow);
  void drawLastLineSelection(QPainter *painter, const ViewportContext &ctx,
                             size_t endRow, size_t endCol);

  void increaseFontSize();
  void decreaseFontSize();
  void resetFontSize();

  void handleViewportUpdate();
  void scrollToCursor();
  double measureContent();

  NekoConfigManager *configManager;
  NekoThemeManager *themeManager;
  NekoEditor *editor;
  QFont font;
  QFontMetricsF fontMetrics;

  int EXTRA_VERTICAL_LINES = 1;
  double FONT_STEP = 2.0;
  double DEFAULT_FONT_SIZE = 15.0;
  double FONT_UPPER_LIMIT = 96.0;
  double FONT_LOWER_LIMIT = 6.0;
  QColor CURSOR_COLOR = QColor(66, 181, 212);
  QColor LINE_HIGHLIGHT_COLOR = QColor(255, 255, 255, 25);
  QColor TEXT_COLOR = QColor(235, 235, 235);
  QColor SELECTION_COLOR = QColor(66, 181, 212, 50);
  double VIEWPORT_PADDING = 74.0;
};

#endif // EDITOR_WIDGET_H
