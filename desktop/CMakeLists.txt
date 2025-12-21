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

    src/features/main_window/main_window.cpp
    src/features/main_window/main_window.h
    src/features/main_window/controllers/app_state_controller.cpp
    src/features/main_window/controllers/app_state_controller.h

    src/features/title_bar/title_bar_widget.cpp
    src/features/title_bar/title_bar_widget.h

    src/features/status_bar/status_bar_widget.cpp
    src/features/status_bar/status_bar_widget.h

    src/features/editor/editor_widget.cpp
    src/features/editor/editor_widget.h
    src/features/editor/gutter_widget.cpp
    src/features/editor/gutter_widget.h
    src/features/editor/render/editor_renderer.cpp
    src/features/editor/render/editor_renderer.h
    src/features/editor/render/gutter_renderer.cpp
    src/features/editor/render/gutter_renderer.h
    src/features/editor/render/render_state.h
    src/features/editor/render/render_theme.h
    src/features/editor/controllers/editor_controller.cpp
    src/features/editor/controllers/editor_controller.h

    src/features/file_explorer/file_explorer_widget.cpp
    src/features/file_explorer/file_explorer_widget.h

    src/features/tabs/tab_bar_widget.cpp
    src/features/tabs/tab_bar_widget.h
    src/features/tabs/tab_widget.cpp
    src/features/tabs/tab_widget.h
    src/features/tabs/controllers/tab_controller.cpp
    src/features/tabs/controllers/tab_controller.h

    src/features/command_palette/command_palette_widget.cpp
    src/features/command_palette/command_palette_widget.h
    src/features/command_palette/palette_frame.cpp
    src/features/command_palette/palette_frame.h
    src/features/command_palette/palette_divider.cpp
    src/features/command_palette/palette_divider.h

    src/features/context_menu/context_menu_widget.cpp
    src/features/context_menu/context_menu_widget.h
    src/features/context_menu/context_menu_item_kind.h
    src/features/context_menu/context_menu_item.h
    src/features/context_menu/command_registry.cpp
    src/features/context_menu/command_registry.h
    src/features/context_menu/context_menu_registry.cpp
    src/features/context_menu/context_menu_registry.h

    src/utils/change_mask.h
    src/utils/editor_utils.cpp
    src/utils/editor_utils.h
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
