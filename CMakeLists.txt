cmake_minimum_required(VERSION 3.21)
project(sprixion VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Test LinguistTools)
qt_standard_project_setup()
enable_testing()

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/W4> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>)

# spx_test(<name>) builds tests/core/<name>.cpp against sprixion_core and registers it with CTest.
function(spx_test name)
    add_executable(${name} ${CMAKE_SOURCE_DIR}/tests/core/${name}.cpp)
    target_link_libraries(${name} PRIVATE sprixion_core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_subdirectory(src/core)
add_subdirectory(src/app)
add_subdirectory(tests)
