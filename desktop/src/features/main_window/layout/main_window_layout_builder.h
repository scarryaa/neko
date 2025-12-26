#ifndef MAIN_WINDOW_LAYOUT_BUILDER_H
#define MAIN_WINDOW_LAYOUT_BUILDER_H

class ThemeProvider;
class UiStyleManager;

#include "types/ffi_types_fwd.h"
#include "types/qt_types_fwd.h"
#include <QObject>

QT_FWD(QPushButton, QSplitter)

class MainWindowLayoutBuilder {
public:
  struct MainWindowLayoutProps {
    ThemeProvider *themeProvider;
    neko::ConfigManager *configManager;
  };

  struct MainWindowLayoutWidgets {
    QWidget *titleBarWidget;
    QWidget *tabBarWidget;
    QWidget *editorWidget;
    QWidget *gutterWidget;
    QWidget *fileExplorerWidget;
    QWidget *statusBarWidget;
  };

  struct MainWindowLayoutResult {
    QWidget *centralWidget;
    QWidget *tabBarContainer;
    QPushButton *newTabButton;
    QWidget *emptyStateWidget;
    QPushButton *emptyStateNewTabButton;
    QSplitter *mainSplitter;
  };

  explicit MainWindowLayoutBuilder(const MainWindowLayoutProps &props,
                                   QWidget *parent = nullptr);

  MainWindowLayoutResult build(const MainWindowLayoutWidgets &widgets);

private:
  MainWindowLayoutProps props;

  struct TabBarSectionResult {
    QWidget *tabBarContainer;
    QPushButton *newTabButton;
  };

  struct EmptyStateSectionResult {
    QWidget *emptyStateWidget;
    QPushButton *newTabButton;
  };

  TabBarSectionResult buildTabBarSection(QWidget *tabBarWidget);
  EmptyStateSectionResult buildEmptyStateSection();
  QWidget *buildEditorSection(QWidget *gutterWidget, QWidget *editorWidget,
                              QWidget *emptyState);
  QSplitter *buildSplitter(QWidget *editorSideContainer,
                           QWidget *fileExplorerWidget);

  QWidget *rootParent;
};

#endif
