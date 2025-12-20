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
#include <QTimer>
#include <QVBoxLayout>
#include <QtDebug>

class EditorWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit EditorWidget(EditorController *editorController,
                        neko::ConfigManager &configManager,
                        neko::ThemeManager &themeManager,
                        QWidget *parent = nullptr);
  ~EditorWidget();

  void applyTheme();
  void redraw() const;
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
  void mouseReleaseEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  bool focusNextPrevChild(bool next) override;

signals:
  void fontSizeChanged(const qreal newSize);
  void newTabRequested();

private:
  const double getTextWidth(const QString &text,
                            const double horizontalOffset) const;
  const RowCol convertMousePositionToRowCol(const double x, const double y);

  void increaseFontSize();
  void decreaseFontSize();
  void resetFontSize();
  void setFontSize(const double newFontSize);

  void scrollToCursor();
  const double measureWidth() const;

  neko::ConfigManager &configManager;
  neko::ThemeManager &themeManager;
  EditorController *editorController;
  EditorRenderer *renderer;
  QFont font;
  QFontMetricsF fontMetrics;

  QTimer suppressDblTimer;
  bool suppressNextDouble = false;
  QPoint suppressDblPos{};
  QTimer tripleArmTimer;
  bool tripleArmed = false;
  QPoint triplePos{};
  int tripleRow = 0;
  bool lineSelectMode = false;
  bool wordSelectMode = false;
  RowCol wordAnchorStart{0, 0};
  RowCol wordAnchorEnd{0, 0};
  int lineAnchorRow = 0;

  const int EXTRA_VERTICAL_LINES = 1;
  const double FONT_STEP = 2.0;
  const double DEFAULT_FONT_SIZE = 15.0;
  const double FONT_UPPER_LIMIT = 96.0;
  const double FONT_LOWER_LIMIT = 6.0;
  const double VIEWPORT_PADDING = 74.0;

  static constexpr int TRIPLE_CLICK_MS = 200;
};

#endif // EDITOR_WIDGET_H
