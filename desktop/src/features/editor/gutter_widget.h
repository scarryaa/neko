#ifndef GUTTER_WIDGET_H
#define GUTTER_WIDGET_H

#include "controllers/editor_controller.h"
#include "features/editor/render/render_state.h"
#include "neko-core/src/ffi/mod.rs.h"
#include "render/gutter_renderer.h"
#include "utils/editor_utils.h"
#include "utils/gui_utils.h"
#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QString>
#include <QWheelEvent>

class GutterWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit GutterWidget(EditorController *editorController,
                        neko::ConfigManager &configManager,
                        neko::ThemeManager &themeManager,
                        QWidget *parent = nullptr);
  ~GutterWidget();

  void applyTheme();
  void updateDimensions();

protected:
  QSize sizeHint() const override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

public slots:
  void onEditorFontSizeChanged(const qreal newSize);
  void onEditorLineCountChanged();
  void onEditorCursorPositionChanged();

  void onBufferChanged();
  void onCursorChanged();
  void onSelectionChanged();
  void onViewportChanged();

private:
  const double measureWidth() const;
  void redraw() const;
  void drawText(QPainter *painter, const ViewportContext &ctx,
                const int lineCount);
  void drawLineHighlight(QPainter *painter, const ViewportContext &ctx);

  void decreaseFontSize();
  void increaseFontSize();
  void resetFontSize();

  neko::ThemeManager &themeManager;
  neko::ConfigManager &configManager;
  EditorController *editorController;
  GutterRenderer *renderer;
  QFont font;
  QFontMetricsF fontMetrics;

  const int EXTRA_VERTICAL_LINES = 1;
  const double FONT_STEP = 2.0;
  const double DEFAULT_FONT_SIZE = 15.0;
  const double FONT_UPPER_LIMIT = 96.0;
  const double FONT_LOWER_LIMIT = 6.0;
  const double VIEWPORT_PADDING = 74.0;
};

#endif // GUTTER_WIDGET_H
