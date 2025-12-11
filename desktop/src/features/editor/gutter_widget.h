#ifndef GUTTER_WIDGET_H
#define GUTTER_WIDGET_H

#include "utils/editor_utils.h"
#include "utils/gui_utils.h"
#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QString>
#include <QWheelEvent>
#include <neko_core.h>

class GutterWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit GutterWidget(NekoEditor *editor, NekoConfigManager *configManager,
                        NekoThemeManager *themeManager,
                        QWidget *parent = nullptr);
  ~GutterWidget();

  void updateDimensionsAndRepaint();
  void setEditor(NekoEditor *editor);

protected:
  QSize sizeHint() const override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

public slots:
  void onEditorFontSizeChanged(qreal newSize);
  void onEditorLineCountChanged();
  void onEditorCursorPositionChanged();

private:
  double measureContent() const;
  void handleViewportUpdate();
  void drawText(QPainter *painter, const ViewportContext &ctx, int lineCount);
  void drawLineHighlight(QPainter *painter, const ViewportContext &ctx);

  void decreaseFontSize();
  void increaseFontSize();
  void resetFontSize();

  NekoThemeManager *themeManager;
  NekoConfigManager *configManager;
  NekoEditor *editor;
  QFont font;
  QFontMetricsF fontMetrics;

  double FONT_STEP = 2.0;
  double DEFAULT_FONT_SIZE = 15.0;
  double FONT_UPPER_LIMIT = 96.0;
  double FONT_LOWER_LIMIT = 6.0;
  double VIEWPORT_PADDING = 74.0;
  QColor TEXT_COLOR = QColor(80, 80, 80);
  QColor CURRENT_LINE_COLOR = QColor(200, 200, 200);
  QColor LINE_HIGHLIGHT_COLOR = QColor(255, 255, 255, 25);
};

#endif // GUTTER_WIDGET_H
