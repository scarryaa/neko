find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)
qt_standard_project_setup()

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
    src/features/titlebar/titlebar_widget.cpp
    src/features/titlebar/titlebar_widget.h
    src/features/editor/editor_widget.cpp
    src/features/editor/editor_widget.h
    src/features/editor/gutter_widget.cpp
    src/features/editor/gutter_widget.h
    src/features/file_explorer/file_explorer_widget.cpp
    src/features/file_explorer/file_explorer_widget.h
    src/features/tabs/tab_bar_widget.cpp
    src/features/tabs/tab_bar_widget.h
    src/features/tabs/tab_widget.cpp
    src/features/tabs/tab_widget.h

    src/utils/editor_utils.cpp
    src/utils/editor_utils.h

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
        neko_core  # Link the Rust library

        ${PLATFORM_LIBS}
)

target_include_directories(neko PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
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
