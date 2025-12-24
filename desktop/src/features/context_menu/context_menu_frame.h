#ifndef CONTEXT_MENU_FRAME_H
#define CONTEXT_MENU_FRAME_H

#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <neko-core/src/ffi/bridge.rs.h>

class ContextMenuFrame : public QFrame {
  Q_OBJECT

private:
  struct ContextMenuFrameTheme {
    QString backgroundColor;
    QString borderColor;
  };

public:
  explicit ContextMenuFrame(const ContextMenuFrameTheme &theme,
                            QWidget *parent = nullptr);
  ~ContextMenuFrame() override = default;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  ContextMenuFrameTheme theme;
};

#endif // CONTEXT_MENU_FRAME_H
