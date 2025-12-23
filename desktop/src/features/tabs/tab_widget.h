#ifndef TAB_WIDGET_H
#define TAB_WIDGET_H

#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/context_menu/context_menu_widget.h"
#include "features/context_menu/providers/tab_context.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include "utils/gui_utils.h"
#include <QApplication>
#include <QDrag>
#include <QIcon>
#include <QMimeData>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QWidget>

class TabWidget : public QWidget {
  Q_OBJECT

public:
  using GetTabCountFn = std::function<int()>;

  explicit TabWidget(const QString &title, QString path, int index, int tabId,
                     bool isPinned, neko::ConfigManager &configManager,
                     neko::ThemeManager &themeManager,
                     ContextMenuRegistry &contextMenuRegistry,
                     CommandRegistry &commandRegistry,
                     GetTabCountFn getTabCount, QWidget *parent = nullptr);
  ~TabWidget() override = default;

  void setActive(bool active);
  void setModified(bool modified);
  void setIsPinned(bool isPinned);
  [[nodiscard]] bool getIsPinned() const;
  [[nodiscard]] int getId() const;

signals:
  void clicked();
  void closeRequested(bool bypassConfirmation);
  void unpinRequested();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;

private:
  double measureText(const QString &text);

  [[nodiscard]] QRect closeRect() const;
  [[nodiscard]] QRect closeHitRect() const;
  [[nodiscard]] QRect titleRect() const;
  [[nodiscard]] QRect modifiedRect() const;

  GetTabCountFn getTabCount_;
  ContextMenuRegistry &contextMenuRegistry;
  CommandRegistry &commandRegistry;
  ContextMenuWidget *contextMenuWidget;
  neko::ConfigManager &configManager;
  neko::ThemeManager &themeManager;
  QString title;
  QString path;
  bool isModified = false;
  int index;
  int tabId;
  bool isPinned = false;
  bool isActive;
  bool isHovered = false;
  bool isCloseHovered = false;
  bool dragEligible = false;
  bool dragInProgress = false;
  QPoint dragStartPosition;

  static constexpr int LEFT_PADDING_PX = 12;

  static constexpr int CLOSE_BUTTON_SIZE_PX = 12;
  static constexpr int CLOSE_BUTTON_RIGHT_INSET_PX = 24;
  static constexpr int CLOSE_HIT_INFLATE_PX = 3;

  // How much horizontal room on the right to reserve so text doesn't overlap
  static constexpr int RIGHT_RESERVED_FOR_CONTROLS_PX = 30;
  static constexpr int MIN_RIGHT_EXTRA_PX = 44;

  // Modified dot
  static constexpr int MODIFIED_DOT_SIZE_PX = 6;
  static constexpr int MODIFIED_DOT_RIGHT_INSET_PX = 37;

  // Close "X" padding inside the close rect
  static constexpr int CLOSE_GLYPH_INSET_PX = 2;

  // Pin icon rendering
  static constexpr int PIN_ICON_SIZE_PX = 12;
  static constexpr int PIN_ICON_NUDGE_Y_PX = 1;

  // Visuals
  static constexpr double TOP_PADDING = 8.0;
  static constexpr double BOTTOM_PADDING = 8.0;
  static constexpr double CLOSE_PEN_THICKNESS = 1.5;
  static constexpr double CLOSE_HOVER_ALPHA = 60.0;
};

#endif // TAB_WIDGET_H
