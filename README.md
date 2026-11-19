# Sprixion

A pixel-art editor with layers and animation, written in C++20 with Qt 6 Widgets. Licensed under GPLv3.

## Build

Requires CMake 3.21+, Ninja, a C++20 compiler and Qt 6.5+ (Widgets, Test, LinguistTools).

    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build build
    ctest --test-dir build --output-on-failure
    ./build/src/app/sprixion
