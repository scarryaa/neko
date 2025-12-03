#ifndef EDITORWIDGET_H
#define EDITORWIDGET_H

#include <QWidget>

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
};

#endif
