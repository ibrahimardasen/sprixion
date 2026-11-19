add_library(sprixion_core STATIC
    Sprite.cpp
    SpriteOps.cpp
    Document.cpp
    Commands.cpp
    Raster.cpp
    Compositor.cpp
    io/SprxIO.cpp
    ../../third_party/miniz/miniz.c
)
target_include_directories(sprixion_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(sprixion_core SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/third_party/miniz)
target_link_libraries(sprixion_core PUBLIC Qt6::Core Qt6::Gui)

# miniz is vendored third-party code: no warnings.
set_source_files_properties(../../third_party/miniz/miniz.c PROPERTIES COMPILE_OPTIONS "$<IF:$<C_COMPILER_ID:MSVC>,/w,-w>")
