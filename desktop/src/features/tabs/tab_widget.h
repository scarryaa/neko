#ifndef TAB_WIDGET_H
#define TAB_WIDGET_H

class CommandRegistry;
class ContextMenuRegistry;
class ContextMenuWidget;
class ThemeProvider;

#include "theme/types/types.h"
#include "types/ffi_types_fwd.h"
#include "types/qt_types_fwd.h"
#include <QPoint>
#include <QRect>
#include <QString>
#include <QWidget>

QT_FWD(QPaintEvent, QMouseEvent, QEnterEvent, QEvent, QContextMenuEvent)

class TabWidget : public QWidget {
  Q_OBJECT

public:
  struct TabProps {
    QString title;
    QString path;
    int index;
    int tabId;
    bool isPinned;
    QFont font;
    ThemeProvider *themeProvider;
    TabTheme theme;
    ContextMenuRegistry *contextMenuRegistry;
    CommandRegistry *commandRegistry;
  };

  explicit TabWidget(const TabProps &props, QWidget *parent = nullptr);
  ~TabWidget() override = default;

  [[nodiscard]] bool getIsPinned() const;
  [[nodiscard]] int getId() const;
  [[nodiscard]] bool getIsModified() const;
  [[nodiscard]] QString getPath() const;
  [[nodiscard]] QString getTitle() const;

  void setActive(bool active);
  void setModified(bool modified);
  void setIsPinned(bool isPinned);
  void setAndApplyTheme(const TabTheme &newTheme);
  void setTitle(const QString &newTitle);
  void setPath(const QString &newPath);
  void setIndex(int newIndex);

signals:
  void clicked(int tabId);
  void closeRequested(neko::CloseTabOperationTypeFfi operationType, int tabId,
                      bool bypassConfirmation);
  void unpinRequested(int tabId);

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

  ContextMenuRegistry &contextMenuRegistry;
  CommandRegistry &commandRegistry;
  ThemeProvider *themeProvider;
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
  bool middleClickPending = false;
  QPoint dragStartPosition;

  QFont font;
  TabTheme theme;

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
};

#endif // TAB_WIDGET_H
