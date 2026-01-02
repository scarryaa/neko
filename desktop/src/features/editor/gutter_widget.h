#ifndef GUTTER_WIDGET_H
#define GUTTER_WIDGET_H

#include "render/gutter_renderer.h"
#include "theme/types/types.h"
#include "types/qt_types_fwd.h"
#include <QFont>
#include <QFontMetricsF>
#include <QScrollArea>
#include <QSize>

QT_FWD(QWheelEvent, QPaintEvent)

class EditorBridge;

class GutterWidget : public QScrollArea {
  Q_OBJECT

public:
  struct GutterProps {
    EditorBridge *editorBridge;
    GutterTheme theme;
    QFont font;
  };

  explicit GutterWidget(const GutterProps &props, QWidget *parent = nullptr);
  ~GutterWidget() override = default;

  void redraw() const;
  void setAndApplyTheme(const GutterTheme &newTheme);
  void updateDimensions();
  void setEditorBridge(EditorBridge *newEditorBridge);

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

  void decreaseFontSize();
  void increaseFontSize();
  void resetFontSize();

  GutterTheme theme;

  EditorBridge *editorBridge;
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
