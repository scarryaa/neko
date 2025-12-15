#ifndef EDITOR_WIDGET_H
#define EDITOR_WIDGET_H

#include "neko-core/src/ffi/mod.rs.h"
#include "utils/change_mask.h"
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
#include <QTextLayout>
#include <QVBoxLayout>
#include <QtDebug>

class EditorWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit EditorWidget(neko::Editor *editor,
                        neko::ConfigManager &configManager,
                        neko::ThemeManager &themeManager,
                        QWidget *parent = nullptr);
  ~EditorWidget();

  void updateDimensionsAndRepaint();
  void setEditor(neko::Editor *editor);

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
  void cursorPositionChanged(int row, int col);
  void newTabRequested();
  void closeTabRequested();
  void bufferChanged();

private:
  double getTextWidth(const QString &text, double horizontalOffset) const;
  RowCol convertMousePositionToRowCol(double x, double y);
  void addCursor(neko::AddCursorDirectionKind dirKind, int row = 0,
                 int col = 0);

  void applyChangeSet(const neko::ChangeSetFfi &cs);
  void handleCopy();
  void handleCut();
  void handlePaste();
  void handleUndo();
  void handleRedo();

  void drawText(QPainter *painter, const ViewportContext &ctx);
  void drawCursors(QPainter *painter, const ViewportContext &ctx);
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
  void setFontSize(double newFontSize);

  void handleViewportUpdate();
  void scrollToCursor();
  double measureContent();

  neko::ConfigManager &configManager;
  neko::ThemeManager &themeManager;
  neko::Editor *editor;
  QFont font;
  QFontMetricsF fontMetrics;

  int EXTRA_VERTICAL_LINES = 1;
  double FONT_STEP = 2.0;
  double DEFAULT_FONT_SIZE = 15.0;
  double FONT_UPPER_LIMIT = 96.0;
  double FONT_LOWER_LIMIT = 6.0;
  QColor LINE_HIGHLIGHT_COLOR = QColor(255, 255, 255, 25);
  QColor TEXT_COLOR = QColor(235, 235, 235);
  double VIEWPORT_PADDING = 74.0;
};

#endif // EDITOR_WIDGET_H
