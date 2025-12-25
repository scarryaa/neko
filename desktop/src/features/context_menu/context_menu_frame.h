#ifndef CONTEXT_MENU_FRAME_H
#define CONTEXT_MENU_FRAME_H

#include <QFrame>
#include <QPaintEvent>
#include <QString>

class ContextMenuFrame : public QFrame {
  Q_OBJECT

private:
  struct ContextMenuFrameTheme {
    QString backgroundColor;
    QString borderColor;
  };

  struct ContextMenuFrameProps {
    const ContextMenuFrameTheme &theme;
  };

public:
  explicit ContextMenuFrame(const ContextMenuFrameProps &props,
                            QWidget *parent = nullptr);
  ~ContextMenuFrame() override = default;

  void setAndApplyTheme(const ContextMenuFrameTheme &newTheme);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  ContextMenuFrameTheme theme;
};

#endif // CONTEXT_MENU_FRAME_H
