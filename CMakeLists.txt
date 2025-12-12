cmake_minimum_required(VERSION 3.19)
project(neko LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(CARGO_EXECUTABLE cargo REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_CMD "build")
    set(RUST_TARGET_SUBDIR "debug")
else()
    set(CARGO_CMD "build" "--release")
    set(RUST_TARGET_SUBDIR "release")
endif()

set(RUST_TARGET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core/target")
set(CXX_BRIDGE_DIR "${RUST_TARGET_DIR}/cxxbridge")

file(MAKE_DIRECTORY "${CXX_BRIDGE_DIR}") 

if(WIN32)
    set(RUST_LIB_NAME "neko_core.lib")
elseif(APPLE)
    set(RUST_LIB_NAME "libneko_core.a")
else()
    set(RUST_LIB_NAME "libneko_core.a")
endif()

set(RUST_LIB_FULL_PATH "${RUST_TARGET_DIR}/${RUST_TARGET_SUBDIR}/${RUST_LIB_NAME}")

add_custom_command(
    OUTPUT "${RUST_LIB_FULL_PATH}"
    COMMAND ${CARGO_EXECUTABLE} ${CARGO_CMD}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/core"
    COMMENT "Building Rust core..."
    USES_TERMINAL
)

add_custom_target(neko_core_build ALL DEPENDS "${RUST_LIB_FULL_PATH}")

add_library(neko_core STATIC IMPORTED GLOBAL)

set_target_properties(neko_core PROPERTIES
    IMPORTED_LOCATION "${RUST_LIB_FULL_PATH}"
    INTERFACE_INCLUDE_DIRECTORIES "${CXX_BRIDGE_DIR}"
)

add_dependencies(neko_core neko_core_build)

if(UNIX AND NOT APPLE)
    target_link_libraries(neko_core INTERFACE pthread dl m)
elseif(APPLE)
    target_link_libraries(neko_core INTERFACE "-framework CoreFoundation" "-framework Security")
elseif(WIN32)
    target_link_libraries(neko_core INTERFACE ws2_32 userenv bcrypt)
endif()

add_subdirectory(desktop)
