#ifndef GUTTER_WIDGET_H
#define GUTTER_WIDGET_H

#include "neko-core/src/ffi/mod.rs.h"
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
  explicit GutterWidget(neko::Editor *editor,
                        neko::ConfigManager &configManager,
                        neko::ThemeManager &themeManager,
                        QWidget *parent = nullptr);
  ~GutterWidget();

  void updateDimensionsAndRepaint();
  void setEditor(neko::Editor *editor);

protected:
  QSize sizeHint() const override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

public slots:
  void onEditorFontSizeChanged(qreal newSize);
  void onEditorLineCountChanged();
  void onEditorCursorPositionChanged();

  void onBufferChanged();
  void onCursorChanged();
  void onSelectionChanged();
  void onViewportChanged();

private:
  double measureContent() const;
  void handleViewportUpdate();
  void drawText(QPainter *painter, const ViewportContext &ctx, int lineCount);
  void drawLineHighlight(QPainter *painter, const ViewportContext &ctx);

  void decreaseFontSize();
  void increaseFontSize();
  void resetFontSize();

  neko::ThemeManager &themeManager;
  neko::ConfigManager &configManager;
  neko::Editor *editor;
  QFont font;
  QFontMetricsF fontMetrics;

  double FONT_STEP = 2.0;
  double DEFAULT_FONT_SIZE = 15.0;
  double FONT_UPPER_LIMIT = 96.0;
  double FONT_LOWER_LIMIT = 6.0;
  double VIEWPORT_PADDING = 74.0;
  QColor TEXT_COLOR = QColor(80, 80, 80);
  QColor CURRENT_LINE_COLOR = QColor(200, 200, 200);
};

#endif // GUTTER_WIDGET_H
