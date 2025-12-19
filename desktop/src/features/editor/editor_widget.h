#ifndef EDITOR_WIDGET_H
#define EDITOR_WIDGET_H

#include "controllers/editor_controller.h"
#include "features/editor/render/editor_renderer.h"
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
                        EditorController *editorController,
                        neko::ConfigManager &configManager,
                        neko::ThemeManager &themeManager,
                        QWidget *parent = nullptr);
  ~EditorWidget();

  void applyTheme();
  void setEditor(neko::Editor *editor);
  void redraw();
  void updateDimensions();

public slots:
  void onBufferChanged();
  void onCursorChanged();
  void onSelectionChanged();
  void onViewportChanged();

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  bool focusNextPrevChild(bool next) override;

signals:
  void fontSizeChanged(qreal newSize);
  void newTabRequested();

private:
  double getTextWidth(const QString &text, double horizontalOffset) const;
  RowCol convertMousePositionToRowCol(double x, double y);

  void increaseFontSize();
  void decreaseFontSize();
  void resetFontSize();
  void setFontSize(double newFontSize);

  void scrollToCursor();
  double measureWidth();

  neko::ConfigManager &configManager;
  neko::ThemeManager &themeManager;
  neko::Editor *editor;
  EditorController *editorController;
  EditorRenderer *renderer;
  QFont font;
  QFontMetricsF fontMetrics;

  int EXTRA_VERTICAL_LINES = 1;
  double FONT_STEP = 2.0;
  double DEFAULT_FONT_SIZE = 15.0;
  double FONT_UPPER_LIMIT = 96.0;
  double FONT_LOWER_LIMIT = 6.0;
  double VIEWPORT_PADDING = 74.0;
};

#endif // EDITOR_WIDGET_H
