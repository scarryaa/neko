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
  void directoryChanged(const std::string &newDirectoryPath);

signals:
  void directorySelectionButtonPressed();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  struct ThemeColors {
    QString buttonTextColor;
    QString buttonPressColor;
    QString buttonHoverColor;
    QString backgroundColor;
    QString borderColor;
  };

  void setupLayout();
  static constexpr double getPlatformTitleBarLeftInset();
  static QString getDisplayNameForDir(const QString &path);
  void getThemeColors();
  QString getStyleSheet();

  neko::ThemeManager &themeManager;
  neko::ConfigManager &configManager;
  QPushButton *m_directorySelectionButton;
  QPoint m_clickPos;
  QString m_currentDir;
  bool m_isDragging = false;
  QPoint m_pressGlobalPos;
  QPoint m_windowStartPos;
  ThemeColors m_themeColors;

  static double constexpr TOP_PADDING = 8.0;
  static double constexpr BOTTOM_PADDING = 8.0;
  static double constexpr RIGHT_CONTENT_MARGIN = 10.0;
  static double constexpr VERTICAL_CONTENT_MARGIN = 5.0;
  static double constexpr MACOS_TRAFFIC_LIGHTS_INSET = 84.0;
  static double constexpr OTHER_PLATFORMS_TRAFFIC_LIGHTS_INSET = 10.0;
};

#endif // TITLE_BAR_WIDGET_H
