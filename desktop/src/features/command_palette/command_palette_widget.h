#ifndef COMMAND_PALETTE_WIDGET_H
#define COMMAND_PALETTE_WIDGET_H

#include <QDialog>
#include <QEvent>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>
#include <neko-core/src/ffi/mod.rs.h>

class CommandPaletteWidget : public QWidget {
  Q_OBJECT

public:
  explicit CommandPaletteWidget(neko::ThemeManager &themeManager,
                                QWidget *parent = nullptr);
  ~CommandPaletteWidget();

protected:
  void showEvent(QShowEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  QWidget *parent;

  neko::ThemeManager &themeManager;

  const double WIDTH = 800.0;
  const double HEIGHT = 300.0;
  const double TOP_OFFSET = 300.0;
  const double SHADOW_X_OFFSET = 0.0;
  const double SHADOW_Y_OFFSET = 5.0;
  const double SHADOW_BLUR_RADIUS = 25.0;
  const double CONTENT_MARGIN = 20.0; // Content margin for drop shadow
};

#endif // COMMAND_PALETTE_WIDGET_H
