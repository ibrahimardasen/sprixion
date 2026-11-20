# Sprixion

**A free pixel-art and sprite-animation editor for Windows and Linux, with a Unity importer built in.**

Draw your sprites, animate them on a timeline, save the `.sprx` file into your Unity project, and get sliced sprites and ready-to-use animation clips automatically. You don't need to export anything or slice it by hand.

> **Status:** early release, version 0.1.1. The core editor and Unity importer work, but expect rough edges. Bug reports are welcome.

## Features

**Drawing**
- Pencil with pixel-perfect mode, eraser, fill, line, rectangle, ellipse, eyedropper
- Horizontal and vertical mirror drawing
- Primary and secondary colors, a color picker, and palettes (import `.gpl` and `.hex`, e.g. from Lospec)
- Selections: move, cut, copy, paste, flip and rotate
- Undo and redo for everything

**Layers and animation**
- Layers with visibility, opacity, rename and reorder
- Timeline with per-frame durations, playback, looping and onion skin
- Tags to name animation ranges such as `walk` and `idle`, with playback limited to a tag

**Files**
- `.sprx` project files: a plain zip with a JSON manifest and PNGs, readable by any tool
- PNG import and export, plus sprite sheet export with JSON metadata
- Autosave with crash recovery. Saves are atomic, so a failed save never damages your file.
- Canvas sizes from 1 to 2048 pixels per side

## Unity integration

The `unity/com.sprixion.importer` package adds a `.sprx` importer to the Unity editor (2022.3 LTS and Unity 6).

Save a `.sprx` into your `Assets/` folder and Unity creates:
- one texture with all frames packed, using pixel-art settings (point filter, no compression, no mipmaps)
- one sprite per frame
- one `AnimationClip` per tag, using your frame durations, ready for a `SpriteRenderer`

For tilesets and icon sheets, set a **slice grid** in Sprixion (*Sprite → Slice Grid…*). Every non-empty cell then becomes its own sprite.

Re-saving in Sprixion updates the assets in Unity automatically. Pixels per unit, pivot and clip looping are set in the importer's inspector.

**Install:** in Unity, open *Window → Package Manager → + → Add package from git URL…* and paste:

```
https://github.com/ibrahimardasen/sprixion.git?path=unity/com.sprixion.importer
```

The importer is editor-only, so nothing from it ends up in your game builds.

## Download

Get the latest build from the [Releases page](https://github.com/ibrahimardasen/sprixion/releases):
- **Linux:** an AppImage. Make it executable and run it.
- **Windows:** a zip and a `setup.exe` installer.

## Building from source

You need CMake 3.21+, Ninja, a C++20 compiler and Qt 6.5+ (Widgets, Svg, Test, LinguistTools).

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/app/sprixion
```

Packaging scripts:
- Linux AppImage: `packaging/linux/build-appimage.sh`
- Windows zip and installer: `packaging/windows/deploy.ps1`

## Translations

Every UI string is translatable. To add a language, copy `translations/sprixion_en.ts` to `sprixion_<lang>.ts`, translate it with Qt Linguist, and add it to `src/app/CMakeLists.txt`.

## License

[GPLv3](LICENSE). Sprixion is free software: you can use, study, share and improve it.
