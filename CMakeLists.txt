cmake_minimum_required(VERSION 3.19)
project(neko LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Rust and Cargo
find_program(CARGO_EXECUTABLE cargo REQUIRED)
find_program(RUSTC_EXECUTABLE rustc REQUIRED)

# Determine build type for Rust
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(RUST_TARGET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core/target/debug")
    set(CARGO_BUILD_FLAG "")
else()
    set(RUST_TARGET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core/target/release")
    set(CARGO_BUILD_FLAG "--release")
endif()

# Set Rust library name based on platform
if(WIN32)
    set(RUST_LIB_NAME "neko_core.lib")
    set(RUST_SHARED_LIB "neko_core.dll")
elseif(APPLE)
    set(RUST_LIB_NAME "libneko_core.a")
    set(RUST_SHARED_LIB "libneko_core.dylib")
else()
    set(RUST_LIB_NAME "libneko_core.a")
    set(RUST_SHARED_LIB "libneko_core.so")
endif()

set(RUST_LIB_PATH "${RUST_TARGET_DIR}/${RUST_LIB_NAME}")

# Build Rust library
add_custom_command(
    OUTPUT ${RUST_LIB_PATH}
    COMMAND ${CARGO_EXECUTABLE} build ${CARGO_BUILD_FLAG}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/core
    COMMENT "Building Rust core library..."
    USES_TERMINAL
)

add_custom_target(neko_core_rust ALL DEPENDS ${RUST_LIB_PATH})

# Create imported library target for Rust
add_library(neko_core STATIC IMPORTED GLOBAL)
set_target_properties(neko_core PROPERTIES
    IMPORTED_LOCATION ${RUST_LIB_PATH}
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/core/include
)
add_dependencies(neko_core neko_core_rust)

# Link system libraries that Rust might need
if(UNIX AND NOT APPLE)
    target_link_libraries(neko_core INTERFACE pthread dl m)
elseif(APPLE)
    target_link_libraries(neko_core INTERFACE "-framework CoreFoundation" "-framework Security")
elseif(WIN32)
    target_link_libraries(neko_core INTERFACE ws2_32 userenv bcrypt)
endif()

# Add desktop subdirectory
add_subdirectory(desktop)
