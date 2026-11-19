qt_add_executable(sprixion main.cpp)
target_link_libraries(sprixion PRIVATE sprixion_core Qt6::Widgets)
qt_add_translations(sprixion TS_FILES ${CMAKE_SOURCE_DIR}/translations/sprixion_en.ts RESOURCE_PREFIX /i18n)
