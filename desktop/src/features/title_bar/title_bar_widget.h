#ifndef TITLE_BAR_WIDGET_H
#define TITLE_BAR_WIDGET_H

#include "theme/types/types.h"
#include "types/qt_types_fwd.h"
#include <QPoint>
#include <QString>
#include <QWidget>
#include <string>

QT_FWD(QPushButton, QMouseEvent, QPaintEvent)

class TitleBarWidget : public QWidget {
  Q_OBJECT

public:
  struct TitleBarProps {
    QFont font;
    TitleBarTheme theme;
  };

  explicit TitleBarWidget(const TitleBarProps &props,
                          QWidget *parent = nullptr);
  ~TitleBarWidget() override = default;

  void setAndApplyTheme(const TitleBarTheme &theme);

signals:
  void directorySelectionButtonPressed();

public slots:
  void directoryChanged(const std::string &newDirectoryPath);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  void setupLayout();
  static constexpr double getPlatformTitleBarLeftInset();
  static QString getDisplayNameForDir(const QString &path);
  QString getStyleSheet();

  TitleBarTheme m_theme;
  QFont m_font;

  QPushButton *m_directorySelectionButton;
  bool m_isDragging = false;
  QPoint m_pressGlobalPos;
  QPoint m_windowStartPos;

  static double constexpr TOP_PADDING = 8.0;
  static double constexpr BOTTOM_PADDING = 8.0;
  static double constexpr RIGHT_CONTENT_INSET = 10.0;
  static double constexpr VERTICAL_CONTENT_INSET = 5.0;
  static double constexpr MACOS_TRAFFIC_LIGHTS_INSET = 84.0;
  static double constexpr OTHER_PLATFORMS_TRAFFIC_LIGHTS_INSET = 10.0;
};

#endif // TITLE_BAR_WIDGET_H
