#ifndef EDITOR_WIDGET_H
#define EDITOR_WIDGET_H

#include "controllers/editor_controller.h"
#include "features/editor/render/editor_renderer.h"
#include "row_col.h"
#include "theme/theme_types.h"
#include "types/ffi_types_fwd.h"
#include "types/qt_types_fwd.h"
#include <QFont>
#include <QFontMetricsF>
#include <QPoint>
#include <QScrollArea>
#include <QString>
#include <QTimer>

QT_FWD(QWheelEvent, QMouseEvent, QKeyEvent, QPaintEvent)

class EditorWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit EditorWidget(EditorController *editorController,
                        neko::ConfigManager &configManager,
                        const EditorTheme &theme, QWidget *parent = nullptr);
  ~EditorWidget() override = default;

  void setAndApplyTheme(const EditorTheme &newTheme);
  void redraw() const;
  void updateDimensions();
  void setEditorController(EditorController *newEditorController);

  // NOLINTNEXTLINE
public slots:
  void onBufferChanged() const;
  void onCursorChanged();
  void onSelectionChanged() const;
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
  void fontSizeChanged(qreal newSize);
  void newTabRequested();

private:
  [[nodiscard]] double getTextWidth(const QString &text,
                                    double horizontalOffset) const;
  RowCol convertMousePositionToRowCol(double xPos, double yPos);

  void increaseFontSize();
  void decreaseFontSize();
  void resetFontSize();
  void setFontSize(double newFontSize);

  void scrollToCursor();
  [[nodiscard]] double measureWidth() const;

  EditorTheme theme;

  neko::ConfigManager &configManager;
  EditorController *editorController;
  EditorRenderer *renderer;
  QFont font;
  QFontMetricsF fontMetrics;

  QTimer suppressDblTimer;
  bool suppressNextDouble = false;
  QPoint suppressDblPos;
  QTimer tripleArmTimer;
  bool tripleArmed = false;
  QPoint triplePos;
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
