# Sprixion — Design

Date: 2026-11-19
Status: draft, awaiting owner approval

## Intent

Sprixion is a free, open-source desktop app for 2D pixel art and frame-by-frame sprite animation, for Windows and Linux, written in C++.

Why it exists:

1. **Dogfooding.** The owner is starting a Unity 2D pixel-art game and will make all its art in Sprixion. The game also advertises the tool ("made with Sprixion").
2. **A free alternative.** The leader in this space (Aseprite) is paid and its license forbids reuse. LibreSprite is stuck on an old fork, and Pixelorama is often called heavy. Sprixion aims to be "as good as Aseprite, free and actively developed", then add what the others lack.

The owner is new to game dev and Unity. They delegated MVP scope and all technical choices; those are marked *(builder's call, made during design)* where relevant.

## Success criteria

- Every sprite and animation in the owner's Unity game can be made in Sprixion without needing another art tool.
- Saving a project file into a Unity project's `Assets/` folder yields sliced sprites and ready-to-use animation clips with no manual setup.
- The app looks and feels like a polished product, not a dev tool (it is the showcase).

## Goals (MVP)

**Drawing tools**
- Pencil with a pixel-perfect mode (removes L-shaped corner pixels on freehand strokes)
- Eraser
- Bucket fill (contiguous; tolerance 0 is enough for MVP)
- Line, rectangle, ellipse (outline and filled)
- Eyedropper
- Rectangular selection, move selection contents
- Copy, cut, paste (within and between sprites; paste PNG from system clipboard)
- Flip horizontal/vertical and rotate 90°/180° for a selection, a layer, or the whole sprite
- Mirror (symmetry) drawing: horizontal and/or vertical axis through canvas center; applies to pencil, eraser, line, rectangle and ellipse
- Primary/secondary color (left/right mouse button)

*(Copy/paste, flip/rotate and canvas resize/crop were added during design review: the success criterion "no other art tool needed" wins over a shorter list.)*

**Canvas**
- Zoom (integer steps, nearest-neighbor) and pan
- Toggleable pixel grid
- New sprite dialog with width/height (limit 1–2048 per side)
- Canvas resize (with anchor) and crop to selection
- Transparent background shown as a checkerboard

**Layers**
- Add, delete, reorder, rename, show/hide, per-layer opacity
- Normal blend mode only

**Palette and color**
- Color picker (HSV + hex input + alpha)
- Palette panel (click to select, add/remove swatches)
- Import palettes: `.gpl` (GIMP) and `.hex` (Lospec)

**Animation**
- Timeline: frames as columns, layers as rows, each cell is a "cel" (may be empty)
- Add, delete, duplicate, reorder frames
- Per-frame duration in milliseconds (default 100)
- Playback in the canvas, with loop
- Onion skin: previous and next frames shown faded, 1–3 frames each way, on/off toggle
- Tags: named frame ranges (e.g. `idle` = frames 1–4, `walk` = 5–12) shown above the timeline; playback can be limited to the selected tag

**Editing**
- Undo/redo for every document change (drawing, layers, frames, tags, palette, canvas size)

**Files**
- Native project file `.sprx` (format below)
- Import PNG as a new sprite; export current frame as PNG
- Export sprite sheet (PNG) with JSON metadata. Layouts: horizontal strip, grid (N columns)
- Autosave to a recovery file every few minutes; offer recovery after a crash

**Unity integration (the differentiator)**
- A Unity package (C#) with a `ScriptedImporter` for `.sprx`. Dropping or saving a `.sprx` into `Assets/` produces:
  - one texture with all frames packed, with import settings suited to pixel art (Point filter, no compression, no mipmaps)
  - one sprite per frame, sliced automatically
  - one `AnimationClip` per tag, using the per-frame durations
  - importer options in the Unity inspector: pixels per unit (default 16), pivot (default bottom-center), loop clips (default on)
- **Grid slicing for non-animated sheets** (tilesets, item icons, UI pieces): each `.sprx` has an optional slice grid (cell width/height, offset, spacing) set in Sprixion and shown as an overlay on the canvas. When set, the importer cuts every frame into one sprite per non-empty cell, named `<file>_<frame>_<index>`. Stored in the manifest from `formatVersion` 1 so it never needs a format migration.
- Importer rules *(builder's calls)*: only visible layers are baked, with layer opacity applied; frames outside any tag still become sprites but get no clip; overlapping tags each get their own clip; clips target `SpriteRenderer.sprite`.
- Re-saving in Sprixion updates the assets in Unity automatically (normal Unity reimport)

## Non-goals (later versions)

- Tilemap mode
- Opening Aseprite `.ase`/`.aseprite` files
- Plugins / scripting
- Effects (outline, color replace, shading tools)
- Indexed color mode (MVP is RGBA only)
- Export to other engines (Godot etc.) beyond the generic sprite sheet + JSON
- Languages other than English (but see i18n constraint)

## Anti-goals

- Vector drawing, realistic brushes, 3D. These blur what the product is.
- A UI that looks like a debug tool.
- Losing user work (crash or bad save). Saves must be atomic (write temp file, then rename).
- An endless feature list that never ships. If it isn't needed for the owner's game, it waits.

## Constraints

- **Language:** C++ (C++20).
- **Platforms:** Windows 10+ and Linux (x86_64). No macOS.
- **License:** GPLv3 *(proposed during design, owner did not object)*.
- **UI language:** English only for now, built for translation from day one: all user-visible strings go through Qt's `tr()`, set up with Qt Linguist (`.ts` files) in the build, no hard-coded user-facing text outside `tr()`. Adding a language must only require a new `.ts` file.
- **No code from Aseprite** (license). Ideas and UX conventions are fine.
- **Performance:** smooth drawing and playback for a 256×256 sprite with 10 layers and 100 frames.

## Approaches considered

1. **Qt 6 Widgets — chosen.** Mature cross-platform UI: dockable panels, native file dialogs, shortcuts, high-DPI and translation support built in. LGPL is compatible with a GPL app. Cost: large runtime (~30–50 MB) and Qt's own conventions.
2. **Dear ImGui + SDL3.** Light and fast to prototype, but looks like a dev tool, and file dialogs, text input and i18n need extra work. Weak fit for a showcase product.
3. **Custom UI toolkit (Aseprite's approach).** Full control, but months of UI plumbing before the first tool. Rejected: works against "usable for the owner's game soon".

The owner agreed to Qt 6.

## Decided parts of the how

- **Stack:** C++20, Qt 6 Widgets (minimum 6.5 LTS), CMake *(builder's call)*.
- **Structure:** a `core` library (document model, layers, frames, cels, tags, palette, tools' pixel logic, undo commands, file I/O, export) that uses only QtCore/QtGui types (`QImage`, `QColor`, JSON), and an `app` target with all widgets. `core` has unit tests (Qt Test); `app` stays thin.
- **Undo:** command pattern via `QUndoStack`; drawing commands store the changed rectangle of the cel before and after, not whole images.
- **Canvas rendering:** composite visible layers of the current frame into a `QImage`, draw scaled with nearest-neighbor via `QPainter`. Upgrade to OpenGL only if the performance target is missed.
- **Default shortcuts:** follow common pixel-art tool conventions (B pencil, E eraser, G bucket, I eyedropper, M selection, L line, U rectangle/ellipse, Ctrl+Z/Ctrl+Y undo/redo, Enter play/pause, Alt+, / Alt+. previous/next frame).
- **`.sprx` format:** a zip archive containing `manifest.json` (format version, canvas size, layers, frames with durations, tags, palette, optional slice grid, cel references) and one canvas-sized PNG per non-empty cel *(builder's call: canvas-sized cels keep the model simple; trimming can come later behind `formatVersion`)*. Readable with standard zip and PNG libraries, so the Unity importer can read it with `System.IO.Compression` and `Texture2D.LoadImage`. Has a `formatVersion` field for future changes. Zip via vendored miniz *(builder's call)*.
- **Sprite sheet JSON:** use the widely known Aseprite JSON "array" export schema (`frames[].frame`, `duration`, `meta.frameTags`), so existing engine importers and tools (Phaser, Godot plugins etc.) already understand it.
- **Unity package:** separate folder in the repo, installable through Unity Package Manager by git URL; supports Unity 2022.3 LTS and Unity 6.
- **Packaging:** Windows: zip + installer built with `windeployqt`; Linux: AppImage.

## Left to the builder

- Internal class design, file layout, naming
- Exact panel layout and visual theme (dark theme expected for a pixel-art tool; must look polished)
- Sprite sheet packing details within the two layouts
- Autosave interval and recovery UX details
- CI setup (GitHub Actions for both platforms is the natural choice)
- Test coverage beyond: `core` model operations, undo/redo, `.sprx` round-trip, palette import, sprite sheet + JSON export, pixel-perfect pencil and fill algorithms
