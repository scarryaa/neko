#ifndef GUTTER_WIDGET_H
#define GUTTER_WIDGET_H

#include "controllers/editor_controller.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include "render/gutter_renderer.h"
#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QWheelEvent>

class QPaintEvent;
class QWheelEvent;

class GutterWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit GutterWidget(EditorController *editorController,
                        neko::ConfigManager &configManager,
                        neko::ThemeManager &themeManager,
                        QWidget *parent = nullptr);
  ~GutterWidget() override = default;

  void redraw() const;
  void applyTheme();
  void updateDimensions();
  void setEditorController(EditorController *newEditorController);

protected:
  [[nodiscard]] QSize sizeHint() const override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

public slots:
  void onEditorFontSizeChanged(qreal newSize);
  void onEditorLineCountChanged();
  void onEditorCursorPositionChanged() const;

  void onBufferChanged() const;
  void onCursorChanged() const;
  void onSelectionChanged() const;
  void onViewportChanged();

private:
  [[nodiscard]] double measureWidth() const;
  void drawText(QPainter *painter, const ViewportContext &ctx, int lineCount);
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
