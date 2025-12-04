#ifndef EDITORWIDGET_H
#define EDITORWIDGET_H

#include <QWidget>
#include <neko_core.h>

class EditorWidget : public QWidget {
  Q_OBJECT

public:
  explicit EditorWidget(QWidget *parent = nullptr);
  ~EditorWidget();

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  void drawText(QPainter *painter);

  NekoBuffer *buffer;
  NekoCursor *cursor;
  QFont *font;
};

#endif
