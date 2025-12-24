#ifndef TITLE_BAR_WIDGET_H
#define TITLE_BAR_WIDGET_H

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <neko-core/src/ffi/bridge.rs.h>
#include <string>

class TitleBarWidget : public QWidget {
  Q_OBJECT

public:
  explicit TitleBarWidget(neko::ConfigManager &configManager,
                          neko::ThemeManager &themeManager,
                          QWidget *parent = nullptr);
  ~TitleBarWidget() override = default;

  void applyTheme();

  // NOLINTNEXTLINE
public slots:
  void onDirChanged(const std::string &newDir);

signals:
  void directorySelectionButtonPressed();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

private:
  void onDirectorySelectionButtonPressed();

  neko::ThemeManager &themeManager;
  neko::ConfigManager &configManager;
  double m_height;
  QPushButton *m_directorySelectionButton;
  QPoint m_clickPos;
  QString m_currentDir;

  static double constexpr TOP_PADDING = 8.0;
  static double constexpr BOTTOM_PADDING = 8.0;
  static double constexpr RIGHT_CONTENT_MARGIN = 10.0;
  static double constexpr VERTICAL_CONTENT_MARGIN = 5.0;
};

#endif // TITLE_BAR_WIDGET_H
