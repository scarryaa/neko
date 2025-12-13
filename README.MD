# neko

A cross-platform editor built with Rust and Qt6.

## Prerequisites
- CMake 3.19 or higher
- C++17 compatible compiler
- Rust toolchain (cargo)
- Qt 6.5 or higher

## Quickstart

### macOS
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ./build/desktop/neko.app/Contents/MacOS/neko
```

### Linux
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ./build/desktop/neko
```

### Windows
```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && .\build\desktop\Release\neko.exe
```
