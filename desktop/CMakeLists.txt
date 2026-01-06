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

set(SKIP_UIC_SOURCES
  src/features/main_window/main_window.cpp
  src/features/main_window/connections/file_explorer_connections.cpp
  src/theme/theme_provider.cpp
  src/features/main_window/controllers/ui_style_manager.cpp
  src/features/main_window/connections/ui_style_connections.cpp
  src/utils/ui_utils.cpp
  src/features/command_palette/command_palette_widget.cpp
  src/features/context_menu/context_menu_widget.cpp
  src/features/editor/editor_widget.cpp
  src/features/editor/gutter_widget.cpp
  src/features/file_explorer/file_explorer_widget.cpp
  src/features/status_bar/status_bar_widget.cpp
  src/features/tabs/tab_bar_widget.cpp
  src/features/tabs/tab_widget.cpp
  src/features/title_bar/title_bar_widget.cpp
  src/features/main_window/layout/main_window_layout_builder.cpp
  src/features/main_window/ui_handles.h
  src/features/main_window/connections/theme_connections.h
  src/features/main_window/connections/ui_style_connections.h
  src/features/main_window/connections/main_window_connections.cpp
  src/features/main_window/connections/editor_connections.cpp
  src/features/main_window/connections/workspace_connections.cpp
  src/features/main_window/controllers/shortcuts_manager.cpp
  src/features/main_window/controllers/workspace_coordinator.cpp
  src/features/main_window/controllers/workspace_coordinator.h
  src/features/main_window/main_window.h
  src/features/main_window/flows/tab_flows.cpp
  src/features/main_window/flows/tab_flows.h
  src/features/main_window/flows/file_explorer_flows.cpp
  src/features/main_window/flows/file_explorer_flows.h
)

set_source_files_properties(
  ${SKIP_UIC_SOURCES}
  PROPERTIES SKIP_AUTOUIC ON
)

qt_add_executable(neko
    WIN32 MACOSX_BUNDLE
    src/main.cpp

    # Core Bridges
    src/core/bridge/app_bridge.cpp
    src/core/bridge/app_bridge.h

    # Main Window
    src/features/main_window/main_window.cpp
    src/features/main_window/main_window.h

    # Main Window Layout
    src/features/main_window/layout/main_window_layout_builder.cpp
    src/features/main_window/layout/main_window_layout_builder.h

    # UI Handles
    src/features/main_window/ui_handles.h

    # Main Window Interfaces
    src/features/main_window/interfaces/close_decision.h
    src/features/main_window/interfaces/save_result.h

    # Main Window Controllers
    src/features/main_window/controllers/workspace_coordinator.cpp
    src/features/main_window/controllers/workspace_coordinator.h
    src/features/main_window/controllers/command_manager.cpp
    src/features/main_window/controllers/command_manager.h
    src/features/main_window/controllers/shortcuts_manager.cpp
    src/features/main_window/controllers/shortcuts_manager.h
    src/features/main_window/controllers/ui_style_manager.cpp
    src/features/main_window/controllers/ui_style_manager.h
    src/features/main_window/controllers/command_executor.cpp
    src/features/main_window/controllers/command_executor.h

    # Main Window Services
    src/features/main_window/services/dialog_service.cpp
    src/features/main_window/services/dialog_service.h
    src/features/main_window/services/app_config_service.cpp
    src/features/main_window/services/app_config_service.h
    src/features/main_window/services/file_io_service.cpp
    src/features/main_window/services/file_io_service.h

    # Workspace Coordinator Flows
    src/features/main_window/flows/tab_flows.cpp
    src/features/main_window/flows/tab_flows.h
    src/features/main_window/flows/file_explorer_flows.cpp
    src/features/main_window/flows/file_explorer_flows.h

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
    src/features/main_window/connections/ui_style_connections.cpp
    src/features/main_window/connections/ui_style_connections.h

    # Theme 
    src/theme/theme_provider.cpp
    src/theme/theme_provider.h

    # Theme Types
    src/theme/types/types.h

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
    src/features/editor/render/editor_render_utils.cpp
    src/features/editor/render/editor_render_utils.h

    # Editor Types
    src/features/editor/types/types.h
    src/features/editor/render/types/types.h

    # Editor/Gutter Renderers
    src/features/editor/render/editor_renderer.cpp
    src/features/editor/render/editor_renderer.h
    src/features/editor/render/gutter_renderer.cpp
    src/features/editor/render/gutter_renderer.h

    # Editor Bridge
    src/features/editor/bridge/editor_bridge.cpp
    src/features/editor/bridge/editor_bridge.h

    # File Explorer
    src/features/file_explorer/file_explorer_widget.cpp
    src/features/file_explorer/file_explorer_widget.h

    # File Explorer Controllers
    src/features/file_explorer/controllers/file_explorer_controller.cpp
    src/features/file_explorer/controllers/file_explorer_controller.h

    # File Explorer Bridge
    src/features/file_explorer/bridge/file_tree_bridge.cpp
    src/features/file_explorer/bridge/file_tree_bridge.h

    # Tabs
    src/features/tabs/tab_bar_widget.cpp
    src/features/tabs/tab_bar_widget.h
    src/features/tabs/tab_widget.cpp
    src/features/tabs/tab_widget.h

    # Tab Bridge
    src/features/tabs/bridge/tab_bridge.cpp
    src/features/tabs/bridge/tab_bridge.h

    # Tab Types
    src/features/tabs/types/types.h

    # Command Palette
    src/features/command_palette/command_palette_widget.cpp
    src/features/command_palette/command_palette_widget.h
    src/features/command_palette/palette_frame.cpp
    src/features/command_palette/palette_frame.h
    src/features/command_palette/palette_divider.cpp
    src/features/command_palette/palette_divider.h
    src/features/command_palette/current_size_stacked_widget.cpp
    src/features/command_palette/current_size_stacked_widget.h

    # Command Palette types
    src/features/command_palette/types/types.h

    # Context Menu
    src/features/context_menu/context_menu_widget.cpp
    src/features/context_menu/context_menu_widget.h
    src/features/context_menu/command_registry.cpp
    src/features/context_menu/command_registry.h
    src/features/context_menu/context_menu_registry.cpp
    src/features/context_menu/context_menu_registry.h
    src/features/context_menu/context_menu_frame.cpp
    src/features/context_menu/context_menu_frame.h

    # Context Menu Types
    src/features/context_menu/types/types.h

    # Utils
    src/utils/ui_utils.cpp
    src/utils/ui_utils.h

    # Types
    src/types/command_type.h

    # Forward declarations
    src/types/ffi_types_fwd.h
    src/types/qt_types_fwd.h

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

target_compile_definitions(neko PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
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
