find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)
qt_standard_project_setup()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
  find_library(COCOA_LIB Cocoa)
  list(APPEND PLATFORM_SRCS
    src/utils/mac_utils.mm
    src/utils/mac_utils.h
  )
  list(APPEND PLATFORM_LIBS ${COCOA_LIB})
endif()

qt_add_executable(neko
    WIN32 MACOSX_BUNDLE
    src/main.cpp

    # Main Window
    src/features/main_window/main_window.cpp
    src/features/main_window/main_window.h

    # UI Handles
    src/features/main_window/workspace_ui_handles.h

    # Main Window Interfaces
    src/features/main_window/interfaces/workspace_ui.h
    src/features/main_window/interfaces/close_decision.h
    src/features/main_window/interfaces/save_result.h

    # Main Window Controllers
    src/features/main_window/controllers/app_state_controller.cpp
    src/features/main_window/controllers/app_state_controller.h
    src/features/main_window/controllers/workspace_controller.cpp
    src/features/main_window/controllers/workspace_controller.h
    src/features/main_window/controllers/workspace_coordinator.cpp
    src/features/main_window/controllers/workspace_coordinator.h
    src/features/main_window/controllers/command_manager.cpp
    src/features/main_window/controllers/command_manager.h
    src/features/main_window/controllers/shortcuts_manager.cpp
    src/features/main_window/controllers/shortcuts_manager.h

    # Widget Connections
    src/features/main_window/connections/editor_connections.cpp
    src/features/main_window/connections/editor_connections.h
    src/features/main_window/connections/main_window_connections.cpp
    src/features/main_window/connections/main_window_connections.h
    src/features/main_window/connections/file_explorer_connections.cpp
    src/features/main_window/connections/file_explorer_connections.h
    src/features/main_window/connections/workspace_connections.cpp
    src/features/main_window/connections/workspace_connections.h
    src/features/main_window/connections/theme_connections.cpp
    src/features/main_window/connections/theme_connections.h

    # Theming
    src/theme/theme_provider.cpp
    src/theme/theme_provider.h
    src/theme/theme_types.h

    # Title Bar
    src/features/title_bar/title_bar_widget.cpp
    src/features/title_bar/title_bar_widget.h

    # Status Bar
    src/features/status_bar/status_bar_widget.cpp
    src/features/status_bar/status_bar_widget.h

    # Editor/Gutter
    src/features/editor/editor_widget.cpp
    src/features/editor/editor_widget.h
    src/features/editor/gutter_widget.cpp
    src/features/editor/gutter_widget.h
    src/features/editor/change_mask.h
    src/features/editor/row_col.h
    src/features/editor/row_col.h
    src/features/editor/render/editor_render_utils.cpp
    src/features/editor/render/editor_render_utils.h

    # Editor/Gutter Renderers
    src/features/editor/render/editor_renderer.cpp
    src/features/editor/render/editor_renderer.h
    src/features/editor/render/gutter_renderer.cpp
    src/features/editor/render/gutter_renderer.h
    src/features/editor/render/render_state.h
    src/features/editor/render/render_theme.h

    # Editor Controller
    src/features/editor/controllers/editor_controller.cpp
    src/features/editor/controllers/editor_controller.h

    # File Explorer
    src/features/file_explorer/file_explorer_widget.cpp
    src/features/file_explorer/file_explorer_widget.h

    # File Explorer Controller
    src/features/file_explorer/controllers/file_tree_controller.cpp
    src/features/file_explorer/controllers/file_tree_controller.h

    # Tabs
    src/features/tabs/tab_bar_widget.cpp
    src/features/tabs/tab_bar_widget.h
    src/features/tabs/tab_widget.cpp
    src/features/tabs/tab_widget.h

    # Tab Controller
    src/features/tabs/controllers/tab_controller.cpp
    src/features/tabs/controllers/tab_controller.h

    # Command Palette
    src/features/command_palette/command_palette_widget.cpp
    src/features/command_palette/command_palette_widget.h
    src/features/command_palette/palette_frame.cpp
    src/features/command_palette/palette_frame.h
    src/features/command_palette/palette_divider.cpp
    src/features/command_palette/palette_divider.h

    #Context Menu
    src/features/context_menu/context_menu_widget.cpp
    src/features/context_menu/context_menu_widget.h
    src/features/context_menu/context_menu_item_kind.h
    src/features/context_menu/context_menu_item.h
    src/features/context_menu/command_registry.cpp
    src/features/context_menu/command_registry.h
    src/features/context_menu/context_menu_registry.cpp
    src/features/context_menu/context_menu_registry.h
    src/features/context_menu/context_menu_frame.cpp
    src/features/context_menu/context_menu_frame.h

    # Utils
    src/utils/gui_utils.cpp
    src/utils/gui_utils.h

    ${PLATFORM_SRCS}
)

qt_add_translations(
    TARGETS neko
    TS_FILES translations/neko_en_US.ts
)

target_link_libraries(neko
    PRIVATE
        Qt::Core
        Qt::Widgets
        neko_core
        ${PLATFORM_LIBS}
)

target_include_directories(neko PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include(GNUInstallDirs)
install(TARGETS neko
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET neko
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
