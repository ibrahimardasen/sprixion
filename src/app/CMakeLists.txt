qt_add_executable(sprixion
    main.cpp
    MainWindow.h MainWindow.cpp FileActions.cpp Autosave.h Autosave.cpp
    CanvasView.h CanvasView.cpp
    NewSpriteDialog.h NewSpriteDialog.cpp
    ResizeCanvasDialog.h ResizeCanvasDialog.cpp
    ExportSheetDialog.h ExportSheetDialog.cpp
    LayersPanel.h LayersPanel.cpp PalettePanel.h PalettePanel.cpp ColorPicker.h ColorPicker.cpp
    Timeline.h Timeline.cpp
)
target_link_libraries(sprixion PRIVATE sprixion_core Qt6::Widgets Qt6::Svg) # Svg: makes deploy tools ship the SVG icon plugin
qt_add_resources(sprixion app_resources PREFIX / FILES
    theme.qss
    icons/pencil.svg icons/eraser.svg icons/fill.svg icons/eyedropper.svg icons/select.svg icons/line.svg
    icons/rect.svg icons/ellipse.svg icons/pixel-perfect.svg icons/mirror-h.svg icons/mirror-v.svg icons/filled.svg
)
qt_add_translations(sprixion TS_FILES ${CMAKE_SOURCE_DIR}/translations/sprixion_en.ts RESOURCE_PREFIX /i18n)
