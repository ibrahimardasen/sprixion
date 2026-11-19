# Sprixion MVP Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the Sprixion MVP: a Qt 6 desktop pixel-art and sprite-animation editor for Windows and Linux, plus a Unity importer package for its `.sprx` files.

**Architecture:** A `sprixion_core` static library (QtCore + QtGui only) holds the sprite model as a copyable value type, pure editing operations, pixel algorithms, tools, undo, and all file I/O. Everything there is unit-tested with Qt Test. A thin `sprixion` Qt Widgets app wires the core to the canvas, panels and dialogs. A separate C# Unity package reads `.sprx` files directly.

**Tech Stack:** C++20, Qt 6.5+ (Widgets, Test, LinguistTools), CMake ≥ 3.21 + Ninja, vendored miniz (zip), C# for Unity 2022.3 LTS / Unity 6.

**Spec:** `docs/superpowers/specs/2026-11-19-sprixion-design.md`

## Global Constraints

- C++20; Qt minimum 6.5; Windows 10+ and Linux x86_64 only, no macOS.
- License GPLv3. No code copied from Aseprite.
- Every user-visible string goes through `tr()` (or `QCoreApplication::translate`). English only ships; adding a language must only need a new `.ts` file.
- Canvas size limit: 1–2048 per side. Default frame duration: 100 ms. Color mode: RGBA only (`QImage::Format_ARGB32`).
- Saves are atomic (`QSaveFile`): a failed save never damages the existing file.
- Performance target: smooth drawing and playback at 256×256, 10 layers, 100 frames.
- App version string: `0.1.0`. App name in JSON metadata: `Sprixion`.
- Dark theme, polished look. It is the showcase.

## Review Focus

1. **Corrupt, foreign or newer `.sprx` opened.** Show an error, don't crash, and leave the open document untouched. Test in Task 6.
2. **Save fails (no permission, missing folder).** The old file stays intact and the error is shown. Test in Task 6.
3. **Strokes, fills and pastes that run past the canvas edge.** These are clipped silently. Tests in Tasks 4 and 9.
4. **Deleting the last layer or frame.** At least one of each always remains. Deleting a frame inside or at the edge of a tag shrinks or removes the tag, so no tag ever points past the last frame. Tests in Task 2.
5. **Re-saving a `.sprx` with fewer frames or tags for Unity.** Old sprites and clips vanish on reimport, with no stale assets. This is manual check M3 in Task 15.

---

## File Structure

```
CMakeLists.txt                 root: options, Qt, subdirs, test helper
LICENSE  .gitignore  README.md
third_party/miniz/             miniz.c, miniz.h (MIT, vendored as-is)
src/core/                      library sprixion_core
  Sprite.h/.cpp                value model + makeSprite + celForWrite
  SpriteOps.h/.cpp             pure structural ops (layers, frames, tags, canvas)
  Document.h/.cpp              QObject owning Sprite + QUndoStack
  Commands.h/.cpp              SnapshotCommand, CelPixelsCommand
  Raster.h/.cpp                line, pixel-perfect, rect, ellipse, flood fill, mirror
  Compositor.h/.cpp            flatten a frame
  Selection.h/.cpp             selection + floating pixels
  tools/Tool.h, PencilTool, FillTool, ShapeTool, EyedropperTool, SelectTool (.h/.cpp)
  io/SprxIO.h/.cpp             .sprx load/save
  io/PaletteIO.h/.cpp          .gpl / .hex
  io/SheetExport.h/.cpp        sprite sheet PNG + JSON
src/app/                       executable sprixion
  main.cpp  MainWindow.*  CanvasView.*  LayersPanel.*  PalettePanel.*
  ColorPicker.*  Timeline.*  NewSpriteDialog.*  ResizeCanvasDialog.*
  ExportSheetDialog.*  Autosave.*  theme.qss  resources.qrc
translations/sprixion_en.ts
tests/core/test_*.cpp          one Qt Test executable per file
unity/com.sprixion.importer/   UPM package (package.json, Editor/*.cs, Editor/*.asmdef)
packaging/                     linux AppImage script, windows deploy script + Inno Setup .iss
.github/workflows/build.yml    CI: build + test on ubuntu and windows
```

---

### Task 1: Project skeleton that builds and runs a test

**Files:**
- Create: `CMakeLists.txt`, `src/core/CMakeLists.txt`, `src/app/CMakeLists.txt`, `src/app/main.cpp`, `src/core/Sprite.h` (empty namespace placeholder), `tests/core/test_smoke.cpp`, `translations/sprixion_en.ts`, `third_party/miniz/*`, `LICENSE`, `.gitignore`, `README.md`

**Interfaces:**
- Produces: CMake targets `sprixion_core` (static lib, links `Qt6::Core Qt6::Gui`, includes `src/core`, compiles miniz), `sprixion` (app, links `sprixion_core Qt6::Widgets`), and the CMake function `spx_test(<name>)`. That function builds `tests/core/<name>.cpp` against `sprixion_core Qt6::Test` and registers it with CTest.

- [ ] **Step 1: Install the missing build tools (user runs, needs sudo)**

The machine has g++ 16 and qt6-base 6.12 but no cmake and no Linguist tools. Ask the user to run:
`! sudo pacman -S --needed cmake ninja qt6-tools`
Expected: `cmake --version` prints ≥ 3.21.

- [ ] **Step 2: `git init`, add GPLv3 `LICENSE`, `.gitignore` (`build/`, `*.user`, `.cache/`), and a short `README.md` (what it is, how to build)**

Commit the spec and plan already in `docs/` as the first commit.

- [ ] **Step 3: Write root and sub `CMakeLists.txt`**

`cmake_minimum_required(VERSION 3.21)`, `project(sprixion VERSION 0.1.0 LANGUAGES CXX C)`, `CMAKE_CXX_STANDARD 20`, `find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Test LinguistTools)`, `qt_standard_project_setup()`, `enable_testing()`. In the app: `qt_add_executable(sprixion ...)` and `qt_add_translations(sprixion TS_FILES ${CMAKE_SOURCE_DIR}/translations/sprixion_en.ts RESOURCE_PREFIX /i18n)`. Enable warnings (`-Wall -Wextra` / `/W4`).

- [ ] **Step 4: `main.cpp` shows an empty `QMainWindow` titled `Sprixion`, after installing a `QTranslator` that loads `:/i18n/sprixion_<QLocale::system().name()>` when present**

- [ ] **Step 5: Write `test_smoke.cpp` (one `QVERIFY(true)` slot) and build**

Run: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build && ctest --test-dir build --output-on-failure`
Expected: build succeeds, `1/1 tests passed`. `./build/src/app/sprixion` opens an empty window.

- [ ] **Step 6: Commit** `chore: project skeleton (CMake, Qt 6, tests, i18n)`

---

### Task 2: Sprite model and structural operations

**Files:**
- Create: `src/core/Sprite.h/.cpp`, `src/core/SpriteOps.h/.cpp`
- Test: `tests/core/test_spriteops.cpp`

**Interfaces:**
- Produces (namespace `spx`):
```cpp
struct Layer { QString name; bool visible = true; int opacity = 255; QVector<QImage> cels; }; // cels.size()==frames.size(); null QImage = empty cel
struct Frame { int durationMs = 100; };
struct Tag { QString name; int from = 0; int to = 0; };   // 0-based, inclusive
struct SliceGrid { int cellWidth = 16, cellHeight = 16, offsetX = 0, offsetY = 0, spacingX = 0, spacingY = 0; };
struct Sprite { QSize size; QVector<Layer> layers; /* [0] = bottom */ QVector<Frame> frames;
                QVector<Tag> tags; QVector<QColor> palette; std::optional<SliceGrid> sliceGrid; };
Sprite makeSprite(QSize size);                         // 1 layer "Layer 1", 1 frame, DB16 palette
QImage& celForWrite(Sprite&, int layer, int frame);    // creates transparent ARGB32 canvas-sized cel if null
// SpriteOps.h — all pure: take by value, return the new sprite
Sprite addLayer(Sprite, int index);                   // name "Layer N", N = layers.size()+1
Sprite removeLayer(Sprite, int index);                 // no-op when 1 layer
Sprite moveLayer(Sprite, int from, int to);
Sprite setLayerProps(Sprite, int index, QString name, bool visible, int opacity);
Sprite addFrame(Sprite, int index);                    // empty cels, 100 ms
Sprite duplicateFrame(Sprite, int index);              // copy inserted at index+1
Sprite removeFrame(Sprite, int index);                 // no-op when 1 frame; fixes tags
Sprite moveFrame(Sprite, int from, int to);
Sprite setFrameDuration(Sprite, int index, int ms);    // clamp 1..65535
Sprite addTag(Sprite, Tag); Sprite removeTag(Sprite, int index); Sprite setTag(Sprite, int index, Tag);
Sprite resizeCanvas(Sprite, QSize, Qt::Alignment anchor);   // size clamped to 1..2048
Sprite cropCanvas(Sprite, QRect);                      // rect intersected with canvas
Sprite flipSprite(Sprite, Qt::Orientation);
Sprite rotateSprite(Sprite, int degrees);              // 90/180/270; 90/270 swap width/height
Sprite transformCel(Sprite, int layer, int frame, const QTransform&); // flip/rotate one cel, clipped to canvas
```
DB16 palette: `140c1c 442434 30346d 4e4a4e 854c30 346524 d04648 757161 597dce d27d2c 8595a1 6daa2c d2aa99 6dc2ca dad45e deeed6`.

Tag fix on `removeFrame(i)`: for each tag, a frame after `to` leaves it alone; `from > i` → both shift −1; `from <= i <= to` → `to -= 1`, and if `to < from` the tag is removed.

- [ ] **Step 1: Write failing tests**

```cpp
void makeSpriteDefaults() { auto s = spx::makeSprite({32,32});
  QCOMPARE(s.layers.size(), 1); QCOMPARE(s.layers[0].name, "Layer 1"); QCOMPARE(s.frames.size(), 1);
  QCOMPARE(s.frames[0].durationMs, 100); QCOMPARE(s.palette.size(), 16); QVERIFY(s.layers[0].cels[0].isNull()); }
void cannotRemoveLastLayerOrFrame() { auto s = spx::makeSprite({8,8});
  QCOMPARE(spx::removeLayer(s,0).layers.size(), 1); QCOMPARE(spx::removeFrame(s,0).frames.size(), 1); }
void removeFrameShrinksAndDropsTags() { auto s = spx::makeSprite({8,8});
  for (int i=0;i<5;++i) s = spx::addFrame(s, 1);                    // 6 frames
  s = spx::addTag(s, {"walk",1,3}); s = spx::addTag(s, {"hit",5,5});
  s = spx::removeFrame(s, 2); QCOMPARE(s.tags[0].from,1); QCOMPARE(s.tags[0].to,2);
  s = spx::removeFrame(s, 4); QCOMPARE(s.tags.size(), 1); }        // "hit" (now 4..4) removed
void framesKeepCelsInSync() { auto s = spx::addLayer(spx::makeSprite({8,8}),1);
  s = spx::addFrame(s,1); for (auto& l : s.layers) QCOMPARE(l.cels.size(), 2); }
void duplicateFrameCopiesPixels() { auto s = spx::makeSprite({4,4});
  spx::celForWrite(s,0,0).setPixel(1,1,qRgba(255,0,0,255)); s = spx::duplicateFrame(s,0);
  QCOMPARE(s.layers[0].cels[1].pixel(1,1), qRgba(255,0,0,255)); }
void resizeAnchorsCenter() { auto s = spx::makeSprite({4,4}); spx::celForWrite(s,0,0).setPixel(0,0,qRgba(1,2,3,255));
  s = spx::resizeCanvas(s,{8,8},Qt::AlignCenter); QCOMPARE(s.size, QSize(8,8)); QCOMPARE(s.layers[0].cels[0].pixel(2,2), qRgba(1,2,3,255)); }
void resizeClampsSize() { QCOMPARE(spx::resizeCanvas(spx::makeSprite({4,4}),{5000,0},Qt::AlignCenter).size, QSize(2048,1)); }
void rotate90SwapsSize() { QCOMPARE(spx::rotateSprite(spx::makeSprite({4,8}),90).size, QSize(8,4)); }
void cropOutsideCanvasIsClipped() { QCOMPARE(spx::cropCanvas(spx::makeSprite({8,8}),{4,4,10,10}).size, QSize(4,4)); }
```

- [ ] **Step 2: Run** `ctest --test-dir build -R test_spriteops --output-on-failure`. Expected: FAIL (build error, symbols missing).
- [ ] **Step 3: Implement `Sprite.*` and `SpriteOps.*` per the interfaces.** Use `QImage::copy`/`QPainter` for resize/crop, and `QImage::flipped`/`transformed` for flip/rotate.
- [ ] **Step 4: Run again.** Expected: PASS.
- [ ] **Step 5: Commit** `feat(core): sprite model and structural ops`

---

### Task 3: Document and undo

**Files:**
- Create: `src/core/Document.h/.cpp`, `src/core/Commands.h/.cpp`
- Test: `tests/core/test_document.cpp`

**Interfaces:**
- Consumes: `spx::Sprite`, ops from Task 2.
- Produces:
```cpp
class Document : public QObject { Q_OBJECT
public:
  explicit Document(Sprite s, QObject* parent = nullptr);
  const Sprite& sprite() const;
  QUndoStack* undoStack();
  void apply(const QString& text, Sprite after);   // pushes SnapshotCommand (whole sprite before/after; QImage sharing keeps it cheap)
  QImage& liveCel(int layer, int frame);           // direct pixel access during a stroke (creates cel); caller emits via touch()
  void touch(const QRect& dirty);                  // emits changed(dirty) without undo entry
  void commitCelEdit(const QString& text, int layer, int frame, const QRect& rect, QImage before); // before = rect-sized pixels before the stroke; pushes CelPixelsCommand whose first redo is a no-op
  QString filePath; bool isModified() const;       // !undoStack()->isClean()
signals:
  void changed(QRect dirty);                       // null rect = everything / structure changed
};
```

- [ ] **Step 1: Write failing tests**

```cpp
void applyUndoRedo() { Document d(spx::makeSprite({8,8}));
  d.apply("Add Layer", spx::addLayer(d.sprite(),1)); QCOMPARE(d.sprite().layers.size(),2);
  d.undoStack()->undo(); QCOMPARE(d.sprite().layers.size(),1); d.undoStack()->redo(); QCOMPARE(d.sprite().layers.size(),2); }
void celEditUndoRestoresPixels() { Document d(spx::makeSprite({8,8})); QRect r(2,2,1,1);
  QImage before = d.liveCel(0,0).copy(r); d.liveCel(0,0).setPixel(2,2,qRgba(255,0,0,255));
  d.commitCelEdit("Pencil",0,0,r,before); QCOMPARE(d.sprite().layers[0].cels[0].pixel(2,2), qRgba(255,0,0,255));
  d.undoStack()->undo(); QCOMPARE(qAlpha(d.sprite().layers[0].cels[0].pixel(2,2)), 0);
  d.undoStack()->redo(); QCOMPARE(d.sprite().layers[0].cels[0].pixel(2,2), qRgba(255,0,0,255)); }
void modifiedTracksCleanState() { Document d(spx::makeSprite({8,8})); QVERIFY(!d.isModified());
  d.apply("x", spx::addFrame(d.sprite(),1)); QVERIFY(d.isModified()); d.undoStack()->setClean(); QVERIFY(!d.isModified()); }
void changedSignalFires() { Document d(spx::makeSprite({8,8})); QSignalSpy spy(&d, &Document::changed);
  d.apply("x", spx::addFrame(d.sprite(),1)); QCOMPARE(spy.count(),1); }
```

- [ ] **Step 2: Run** `ctest --test-dir build -R test_document --output-on-failure`. Expected: FAIL.
- [ ] **Step 3: Implement.** The commands are friends of `Document` and swap state through a private `setSprite(Sprite)`, which emits `changed(QRect())`. `CelPixelsCommand` stores `before`/`after` for its rect only and draws them back with `QPainter::CompositionMode_Source`.
- [ ] **Step 4: Run.** Expected: PASS.
- [ ] **Step 5: Commit** `feat(core): document with undo/redo`

---

### Task 4: Pixel algorithms

**Files:**
- Create: `src/core/Raster.h/.cpp`
- Test: `tests/core/test_raster.cpp`

**Interfaces:**
- Produces (namespace `spx::raster`):
```cpp
QVector<QPoint> line(QPoint a, QPoint b);                      // Bresenham, includes both ends
QVector<QPoint> pixelPerfect(const QVector<QPoint>& stroke);   // drops L-corner pixels
QVector<QPoint> rect(QRect r, bool filled);                    // r normalized
QVector<QPoint> ellipse(QRect r, bool filled);                 // midpoint ellipse inscribed in r
QVector<QPoint> mirror(const QVector<QPoint>&, QSize canvas, bool h, bool v); // returns originals + mirrored copies, deduplicated
void plot(QImage& img, const QVector<QPoint>&, QRgb color, QRect* dirty); // skips points outside img; CompositionMode_Source semantics (eraser passes 0)
void floodFill(QImage& img, QPoint seed, QRgb color, QRect* dirty);   // contiguous, 4-way, exact match; no-op if seed outside or same color
```
Pixel-perfect rule (the tests don't fully fix it):
```cpp
// keep p[i] unless p[i-1] and p[i+1] are diagonal neighbours and p[i] touches both orthogonally
for i in 1..n-2: if (abs(p[i-1].x()-p[i+1].x())==1 && abs(p[i-1].y()-p[i+1].y())==1
                     && (p[i].x()==p[i-1].x() || p[i].y()==p[i-1].y())
                     && (p[i].x()==p[i+1].x() || p[i].y()==p[i+1].y())) drop p[i] (and compare the next point against p[i-1])
```
The pencil applies it to the whole stroke so far on every move. Task 7 then redraws the stroke from the saved "before" image.

- [ ] **Step 1: Write failing tests**

```cpp
void lineEndpoints() { auto p = spx::raster::line({0,0},{5,2}); QCOMPARE(p.front(),QPoint(0,0)); QCOMPARE(p.back(),QPoint(5,2)); QCOMPARE(p.size(),6); }
void lineSinglePoint() { QCOMPARE(spx::raster::line({3,3},{3,3}).size(),1); }
void pixelPerfectRemovesCorner() { QVector<QPoint> s{{0,0},{1,0},{1,1}}; QCOMPARE(spx::raster::pixelPerfect(s), (QVector<QPoint>{{0,0},{1,1}})); }
void pixelPerfectKeepsStraight() { QVector<QPoint> s{{0,0},{1,0},{2,0}}; QCOMPARE(spx::raster::pixelPerfect(s), s); }
void ellipseOutline5x5() { auto p = spx::raster::ellipse({0,0,5,5},false);
  for (QPoint q : {QPoint(2,0),QPoint(0,2),QPoint(4,2),QPoint(2,4)}) QVERIFY(p.contains(q)); QVERIFY(!p.contains(QPoint(0,0))); QVERIFY(!p.contains(QPoint(2,2))); }
void ellipseFilledHasCenter() { QVERIFY(spx::raster::ellipse({0,0,5,5},true).contains(QPoint(2,2))); }
void ellipse1x1() { QCOMPARE(spx::raster::ellipse({0,0,1,1},false).size(),1); }
void rectFilledCount() { QCOMPARE(spx::raster::rect({0,0,3,2},true).size(),6); QCOMPARE(spx::raster::rect({0,0,3,3},false).size(),8); }
void mirrorBoth() { auto p = spx::raster::mirror({{0,0}},{4,4},true,true); QCOMPARE(p.size(),4); QVERIFY(p.contains(QPoint(3,3))); }
void plotClipsOutside() { QImage i(4,4,QImage::Format_ARGB32); i.fill(0); QRect d;
  spx::raster::plot(i,{{-1,0},{10,10},{1,1}},qRgba(255,0,0,255),&d); QCOMPARE(d,QRect(1,1,1,1)); }   // no crash, only (1,1)
void fillStopsAtBorder() { QImage i(5,5,QImage::Format_ARGB32); i.fill(0);
  for (int y=0;y<5;++y) i.setPixel(2,y,qRgba(0,0,0,255)); QRect d;
  spx::raster::floodFill(i,{0,0},qRgba(255,0,0,255),&d); QCOMPARE(i.pixel(1,4),qRgba(255,0,0,255)); QCOMPARE(qAlpha(i.pixel(3,0)),0); }
void fillLargeCanvasNoStackOverflow() { QImage i(2048,2048,QImage::Format_ARGB32); i.fill(0); QRect d;
  spx::raster::floodFill(i,{0,0},qRgba(1,1,1,255),&d); QCOMPARE(d,QRect(0,0,2048,2048)); }    // must be iterative/scanline
```

- [ ] **Step 2: Run** `ctest --test-dir build -R test_raster --output-on-failure`. Expected: FAIL.
- [ ] **Step 3: Implement.** The flood fill uses an explicit-stack scanline fill.
- [ ] **Step 4: Run.** Expected: PASS.
- [ ] **Step 5: Commit** `feat(core): raster algorithms`

---

### Task 5: Compositor

**Files:**
- Create: `src/core/Compositor.h/.cpp`
- Test: `tests/core/test_compositor.cpp`

**Interfaces:**
- Produces: `QImage spx::compositeFrame(const Sprite&, int frame);` returns ARGB32, canvas-sized, with visible layers bottom→top, SourceOver, each layer's opacity applied (`opacity/255`).

- [ ] **Step 1: Write failing tests**

```cpp
void hiddenLayerIgnored() { auto s = spx::addLayer(spx::makeSprite({2,2}),1);
  spx::celForWrite(s,1,0).fill(qRgba(255,0,0,255)); s.layers[1].visible = false;
  QCOMPARE(qAlpha(spx::compositeFrame(s,0).pixel(0,0)), 0); }
void opacityApplied() { auto s = spx::makeSprite({1,1}); spx::celForWrite(s,0,0).fill(qRgba(255,0,0,255)); s.layers[0].opacity = 128;
  QVERIFY(qAbs(qAlpha(spx::compositeFrame(s,0).pixel(0,0)) - 128) <= 1); }
void topLayerWins() { auto s = spx::addLayer(spx::makeSprite({1,1}),1);
  spx::celForWrite(s,0,0).fill(qRgba(255,0,0,255)); spx::celForWrite(s,1,0).fill(qRgba(0,0,255,255));
  QCOMPARE(spx::compositeFrame(s,0).pixel(0,0), qRgba(0,0,255,255)); }
```

- [ ] **Step 2: Run** `ctest --test-dir build -R test_compositor`. Expected: FAIL. **Step 3: Implement.** **Step 4: Run.** Expected: PASS.
- [ ] **Step 5: Commit** `feat(core): frame compositor`

---

### Task 6: `.sprx` load/save

**Files:**
- Create: `src/core/io/SprxIO.h/.cpp`
- Test: `tests/core/test_sprxio.cpp`

**Interfaces:**
- Consumes: `Sprite`, `compositeFrame`.
- Produces:
```cpp
bool spx::saveSprx(const Sprite&, const QString& path, QString* error);
std::optional<Sprite> spx::loadSprx(const QString& path, QString* error);
```
Format (the Unity importer in Task 15 depends on it exactly). It is a zip containing:
```json
manifest.json:
{ "formatVersion": 1, "width": 32, "height": 32,
  "layers":  [{"name":"Layer 1","visible":true,"opacity":255}],          // bottom → top
  "frames":  [{"duration":100,"flat":"flat/0.png"}],                     // flat = composited frame
  "tags":    [{"name":"idle","from":0,"to":3}],                          // 0-based inclusive
  "palette": ["#140c1cff", "..."],                                        // #rrggbbaa
  "sliceGrid": null,      // or {"cellWidth":16,"cellHeight":16,"offsetX":0,"offsetY":0,"spacingX":0,"spacingY":0}
  "cels":    [{"layer":0,"frame":0,"file":"cels/0_0.png"}] }             // non-empty cels only
```
`flat/N.png` holds the composited frames, written on every save. *(Plan-time call: Unity then never re-implements compositing, so the import matches the editor exactly.)* Build the zip in memory with miniz (`mz_zip_writer_init_heap`), then write through `QSaveFile` and `commit()`.

Load validation, all rejected with a readable `*error` and `std::nullopt`:
- not a zip, or `manifest.json` missing or not JSON
- `formatVersion` ≠ 1 (message: "This file was made with a newer version of Sprixion.")
- width/height outside 1..2048
- 0 layers or 0 frames
- a cel/tag index out of range, or `from > to`
- a cel PNG that is missing, undecodable, or not canvas-sized
- duration < 1

- [ ] **Step 1: Write failing tests**

```cpp
void roundTrip() { auto s = spx::addFrame(spx::addLayer(spx::makeSprite({16,8}),1),1);
  spx::celForWrite(s,1,1).setPixel(3,4,qRgba(10,20,30,200)); s.layers[1].opacity = 100; s.frames[1].durationMs = 250;
  s = spx::addTag(s,{"walk",0,1}); s.sliceGrid = spx::SliceGrid{8,8,0,0,0,0};
  QTemporaryDir t; QString p = t.filePath("a.sprx"); QString e; QVERIFY(spx::saveSprx(s,p,&e));
  auto l = spx::loadSprx(p,&e); QVERIFY2(l, qPrintable(e));
  QCOMPARE(l->size, QSize(16,8)); QCOMPARE(l->layers[1].cels[1].pixel(3,4), qRgba(10,20,30,200)); QVERIFY(l->layers[0].cels[0].isNull());
  QCOMPARE(l->layers[1].opacity,100); QCOMPARE(l->frames[1].durationMs,250); QCOMPARE(l->tags[0].name,"walk");
  QCOMPARE(l->sliceGrid->cellWidth,8); QCOMPARE(l->palette, s.palette); }
void rejectsGarbage() { QTemporaryDir t; QString p = t.filePath("x.sprx"); QFile f(p); f.open(QIODevice::WriteOnly); f.write("not a zip"); f.close();
  QString e; QVERIFY(!spx::loadSprx(p,&e)); QVERIFY(!e.isEmpty()); }
void rejectsNewerVersion() { QTemporaryDir t; QString p = t.filePath("v2.sprx");
  writeZip(p, {{"manifest.json", R"({"formatVersion":2,"width":4,"height":4,"layers":[{"name":"L","visible":true,"opacity":255}],"frames":[{"duration":100,"flat":"flat/0.png"}],"tags":[],"palette":[],"sliceGrid":null,"cels":[]})"}});
  QString e; QVERIFY(!spx::loadSprx(p,&e)); QVERIFY(e.contains("newer")); }
void rejectsOutOfRangeCel() { QTemporaryDir t; QString p = t.filePath("bad.sprx");
  writeZip(p, {{"manifest.json", R"({"formatVersion":1,"width":4,"height":4,"layers":[{"name":"L","visible":true,"opacity":255}],"frames":[{"duration":100,"flat":"flat/0.png"}],"tags":[],"palette":[],"sliceGrid":null,"cels":[{"layer":5,"frame":0,"file":"cels/5_0.png"}]})"}});
  QString e; QVERIFY(!spx::loadSprx(p,&e)); QVERIFY(!e.isEmpty()); }
void failedSaveKeepsOldFile() { QTemporaryDir t; QString p = t.filePath("a.sprx"); QString e;
  QVERIFY(spx::saveSprx(spx::makeSprite({4,4}),p,&e)); QFile::setPermissions(p, QFile::ReadOwner);
  QFile(t.path()).setPermissions(QFile::ReadOwner|QFile::ExeOwner);             // dir not writable → QSaveFile fails
  QVERIFY(!spx::saveSprx(spx::makeSprite({8,8}),p,&e)); QVERIFY(!e.isEmpty());
  QFile(t.path()).setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner);
  QCOMPARE(spx::loadSprx(p,&e)->size, QSize(4,4)); }                          // skip on Windows (QSKIP) — permission model differs
void saveToMissingFolderFails() { QString e; QVERIFY(!spx::saveSprx(spx::makeSprite({4,4}),"/nonexistent/dir/a.sprx",&e)); QVERIFY(!e.isEmpty()); }
```
The test file gets a small `writeZip(path, QMap<QString,QByteArray>)` helper built on miniz, for hand-made bad manifests.

- [ ] **Step 2: Run** `ctest --test-dir build -R test_sprxio --output-on-failure`. Expected: FAIL. **Step 3: Implement.** **Step 4: Run.** Expected: PASS.
- [ ] **Step 5: Commit** `feat(core): .sprx file format`

---

### Task 7: Tools

**Files:**
- Create: `src/core/tools/Tool.h`, `PencilTool.h/.cpp`, `FillTool.h/.cpp`, `ShapeTool.h/.cpp`, `EyedropperTool.h/.cpp`
- Test: `tests/core/test_tools.cpp`

**Interfaces:**
- Consumes: `Document` (`liveCel`, `touch`, `commitCelEdit`), `spx::raster::*`, `compositeFrame`.
- Produces:
```cpp
struct ToolContext { Document* doc; int layer = 0; int frame = 0; QColor primary{Qt::black}, secondary{Qt::white};
                     bool mirrorH = false, mirrorV = false; bool pixelPerfect = true; bool filled = false;
                     bool constrain = false; };   // Shift held: rect → square, ellipse → circle
class Tool { public: virtual ~Tool() = default;
  virtual void press(ToolContext&, QPoint canvasPos, Qt::MouseButton) = 0;
  virtual void move(ToolContext&, QPoint canvasPos) = 0;
  virtual void release(ToolContext&, QPoint canvasPos) = 0;
  virtual QVector<QPoint> preview() const { return {}; } };     // shapes: points to draw as overlay while dragging
class PencilTool : public Tool { public: explicit PencilTool(bool eraser); ... };
class FillTool : public Tool { ... };
class ShapeTool : public Tool { public: enum Kind { Line, Rect, Ellipse }; explicit ShapeTool(Kind); ... };
class EyedropperTool : public Tool { ... };  // left → ctx.primary, right → ctx.secondary, sampled from compositeFrame
```
Colors: left button = primary, right button = secondary; the eraser writes transparent. Mirroring applies to pencil, eraser, line, rect and ellipse, not to fill. Each completed press–release is exactly one undo entry.

- [ ] **Step 1: Write failing tests**

```cpp
void pencilStrokeIsOneUndo() { Document d(spx::makeSprite({8,8})); spx::ToolContext c{&d}; c.primary = Qt::red; spx::PencilTool p(false);
  p.press(c,{0,0},Qt::LeftButton); p.move(c,{3,0}); p.release(c,{3,0});
  QCOMPARE(d.sprite().layers[0].cels[0].pixel(3,0), QColor(Qt::red).rgba()); QCOMPARE(d.undoStack()->count(),1);
  d.undoStack()->undo(); QCOMPARE(qAlpha(d.sprite().layers[0].cels[0].pixel(3,0)),0); }
void rightButtonUsesSecondary() { Document d(spx::makeSprite({4,4})); spx::ToolContext c{&d}; c.secondary = Qt::blue; spx::PencilTool p(false);
  p.press(c,{1,1},Qt::RightButton); p.release(c,{1,1}); QCOMPARE(d.sprite().layers[0].cels[0].pixel(1,1), QColor(Qt::blue).rgba()); }
void pencilMirror() { Document d(spx::makeSprite({4,4})); spx::ToolContext c{&d}; c.mirrorH = true; spx::PencilTool p(false);
  p.press(c,{0,1},Qt::LeftButton); p.release(c,{0,1}); QCOMPARE(qAlpha(d.sprite().layers[0].cels[0].pixel(3,1)),255); }
void pencilOffCanvasNoCrash() { Document d(spx::makeSprite({4,4})); spx::ToolContext c{&d}; spx::PencilTool p(false);
  p.press(c,{-5,-5},Qt::LeftButton); p.move(c,{10,10}); p.release(c,{10,10}); QCOMPARE(qAlpha(d.sprite().layers[0].cels[0].pixel(2,2)),255); }
void eraserClears() { /* fill cel, erase (1,1), expect alpha 0 */ }
void rectToolOnlyCommitsOnRelease() { Document d(spx::makeSprite({8,8})); spx::ToolContext c{&d}; spx::ShapeTool t(spx::ShapeTool::Rect);
  t.press(c,{0,0},Qt::LeftButton); t.move(c,{3,3}); QVERIFY(!t.preview().isEmpty()); QCOMPARE(d.undoStack()->count(),0);
  t.release(c,{3,3}); QCOMPARE(d.undoStack()->count(),1); QCOMPARE(qAlpha(d.sprite().layers[0].cels[0].pixel(1,1)),0); }
void eyedropperPicksComposite() { /* layer 0 red at (0,0); pick → ctx.primary == red; undo count 0 */ }
void fillTool() { /* 4x4 empty, fill at (0,0) → all 16 pixels primary, 1 undo entry */ }
```

- [ ] **Step 2: Run** `ctest --test-dir build -R test_tools`. Expected: FAIL. **Step 3: Implement.** The pencil keeps the stroke point list plus a "before" copy of the cel. On each move it restores the before pixels in the stroke's bounding rect, re-plots `pixelPerfect(stroke)` (when enabled) plus mirrors, and calls `touch`. On release it calls `commitCelEdit` with the union rect. **Step 4: Run.** Expected: PASS.
- [ ] **Step 5: Commit** `feat(core): drawing tools`

---

### Task 8: Selection, clipboard data, palette import, sprite sheet export

**Files:**
- Create: `src/core/Selection.h/.cpp`, `src/core/tools/SelectTool.h/.cpp`, `src/core/io/PaletteIO.h/.cpp`, `src/core/io/SheetExport.h/.cpp`
- Test: `tests/core/test_selection.cpp`, `tests/core/test_paletteio.cpp`, `tests/core/test_sheetexport.cpp`

**Interfaces:**
- Produces:
```cpp
// Selection.h — the selection is per-document app state, not undoable on its own; lifting/dropping pixels is undoable via commitCelEdit
struct Selection { QRect rect; QImage floating; QPoint floatingPos; bool isActive() const; bool isFloating() const; };
void spx::liftSelection(Document&, Selection&, int layer, int frame);     // move pixels into floating, clear source (1 undo entry)
void spx::dropSelection(Document&, Selection&, int layer, int frame);     // paste floating at floatingPos, clipped (1 undo entry), selection stays as rect
QImage spx::copySelection(const Sprite&, const Selection&, int layer, int frame);
void spx::pasteImage(Document&, Selection&, int layer, int frame, const QImage&, QPoint at); // becomes floating at `at`; larger than canvas → clipped on drop
void spx::transformSelection(Selection&, const QTransform&);              // flip/rotate floating pixels (lifts first via caller)
class SelectTool : public Tool { public: explicit SelectTool(Selection*); ... }; // drag outside selection = new rect; drag inside = lift + move
// PaletteIO.h
std::optional<QVector<QColor>> spx::loadPalette(const QString& path, QString* error); // by extension .gpl / .hex
// SheetExport.h
enum class SheetLayout { Horizontal, Grid };
struct SheetResult { QImage image; QJsonDocument json; };
SheetResult spx::exportSheet(const Sprite&, SheetLayout, int columns, const QString& imageFileName);
```
`.gpl`: the first line must be `GIMP Palette`. Skip `Name:`, `Columns:`, blank lines and `#` lines. Each other line is `R G B [name]`. `.hex`: one `RRGGBB` per line, an optional leading `#` is allowed, blank lines are skipped. A malformed line rejects the whole file and the error names the line number.

Sheet JSON uses the Aseprite "array" schema:
```json
{ "frames": [ { "filename": "<base> 0.png", "frame": {"x":0,"y":0,"w":W,"h":H}, "rotated": false, "trimmed": false,
                "spriteSourceSize": {"x":0,"y":0,"w":W,"h":H}, "sourceSize": {"w":W,"h":H}, "duration": 100 } ],
  "meta": { "app": "Sprixion", "version": "0.1.0", "image": "<imageFileName>", "format": "RGBA8888",
            "size": {"w":..,"h":..}, "scale": "1", "frameTags": [ {"name":"walk","from":0,"to":3,"direction":"forward"} ] } }
```
Grid: `columns` clamped to 1..frameCount, rows = ceil(frames/columns), frames row-major. Horizontal = Grid with columns = frameCount.

- [ ] **Step 1: Write failing tests**

```cpp
// test_selection
void liftMoveDropIsUndoable() { /* pixel at (1,1); select (1,1,1,1); lift; floatingPos=(3,3); drop → (3,3) set, (1,1) clear; undo twice → (1,1) set again */ }
void pastePastCanvasEdgeClipped() { /* 4x4 canvas, paste 4x4 red at (2,2), drop → (3,3) red, no crash, (1,1) untouched */ }
void flipFloating() { /* 2x1 floating [red,blue], flip horizontal → [blue,red] */ }
// test_paletteio
void loadsGpl() { /* "GIMP Palette\nName: x\n#c\n255 0 0 Red\n0 0 255\n" → 2 colors, [0]==red */ }
void loadsHex() { /* "ff0000\n#00ff00\n\n" → 2 colors */ }
void rejectsBadLine() { /* hex "zz0000" → nullopt, error contains "1" */ }
// test_sheetexport
void gridLayout() { auto s = spx::makeSprite({8,4}); for (int i=0;i<4;++i) s = spx::addFrame(s,1); // 5 frames
  auto r = spx::exportSheet(s, spx::SheetLayout::Grid, 2, "a.png"); QCOMPARE(r.image.size(), QSize(16,12));
  auto f = r.json["frames"].toArray(); QCOMPARE(f.size(),5); QCOMPARE(f[3]["frame"]["x"].toInt(),8); QCOMPARE(f[3]["frame"]["y"].toInt(),4); }
void jsonMetaAndTags() { /* tag walk 0..1 → meta.frameTags[0].name=="walk", direction=="forward"; meta.app=="Sprixion"; meta.image=="a.png"; frames[0].duration==100 */ }
void horizontalLayout() { /* 3 frames 8x4 → image 24x4 */ }
```

- [ ] **Step 2: Run** `ctest --test-dir build -R "test_selection|test_paletteio|test_sheetexport"`. Expected: FAIL. **Step 3: Implement.** **Step 4: Run.** Expected: PASS.
- [ ] **Step 5: Commit** `feat(core): selection, palette import, sprite sheet export`

---

### Task 9: Main window, canvas view, file commands

**Files:**
- Create: `src/app/MainWindow.h/.cpp`, `src/app/CanvasView.h/.cpp`, `src/app/NewSpriteDialog.h/.cpp`, `src/app/ResizeCanvasDialog.h/.cpp`, `src/app/ExportSheetDialog.h/.cpp`, `src/app/theme.qss`, `src/app/resources.qrc`
- Modify: `src/app/main.cpp`, `src/app/CMakeLists.txt`

**Interfaces:**
- Consumes: everything from Tasks 2–8.
- Produces:
```cpp
class CanvasView : public QWidget { Q_OBJECT   // paints doc; converts mouse → canvas pixel coords; forwards to current Tool
public:
  void setDocument(Document*); void setTool(Tool*); ToolContext& context(); Selection& selection();
  void setZoom(int factor);  // 1..64, integer only
  void setGridVisible(bool); void setOnionSkin(bool on, int before, int after);   // 1..3 each
signals: void contextChanged(); void zoomChanged(int);
};
class MainWindow : public QMainWindow { Q_OBJECT
public:
  bool openFile(const QString& path);   // shows QMessageBox with loadSprx error on failure; current doc untouched
  bool save(); bool saveAs();           // shows error on failure
  Document* document() const;
};
```
Behavior:
- Painting order: checkerboard (8 px screen squares, `#2b2b2b`/`#3a3a3a`), then onion frames (previous at 30 %/20 %/10 % opacity, next the same), then the composite of the current frame, then the floating selection, then the tool preview. Then the pixel grid (only when zoom ≥ 4) and the selection outline (marching ants).
- Mouse: wheel zooms around the cursor. Middle-drag or Space+drag pans.
- Menus, all through `tr()`:
  - File: New (Ctrl+N), Open (Ctrl+O), Save (Ctrl+S), Save As (Ctrl+Shift+S), Import PNG, Export Frame PNG, Export Sprite Sheet (Ctrl+E), Import Palette, Quit
  - Edit: Undo (Ctrl+Z), Redo (Ctrl+Y and Ctrl+Shift+Z), Cut/Copy/Paste (Ctrl+X/C/V, through `QGuiApplication::clipboard()` image), Select All (Ctrl+A), Deselect (Ctrl+D), Flip H/V, Rotate 90° CW/CCW/180°. These target the floating selection if one exists, otherwise the current cel. Sprite-level versions live in the Sprite menu.
  - Sprite: Resize Canvas, Crop to Selection, Flip/Rotate Sprite, Slice Grid… (edits `sliceGrid` and shows it as an overlay)
  - View: Grid (Ctrl+'), Zoom In/Out (+/−), Onion Skin (F3)
- Tool shortcuts: B pencil, E eraser, G fill, I eyedropper, M select, L line, U cycles rect/ellipse. Tools live in a left toolbar with icons (SVG in `resources.qrc`). Each toolbar also offers the toggles pixel-perfect, mirror H/V and filled shapes.
- New Sprite dialog: width and height spin boxes, 1..2048, default 32×32.
- Window title: `<file name>[*] — Sprixion`, using `setWindowModified`.
- `theme.qss` gives a dark theme applied app-wide in `main.cpp`, together with `QApplication::setStyle("Fusion")`.

- [ ] **Step 1: Build the window and canvas, and wire all menus/actions to core functions**
- [ ] **Step 2: Manual check (record results in the commit message):**
  - draw with every tool, including mirror and pixel-perfect; undo and redo each one
  - zoom with the wheel and pan
  - paste a 64×64 PNG from the system clipboard into a 32×32 sprite: it is clipped, no crash
  - save, reopen and compare
  - open a renamed `.txt` as `.sprx`: an error dialog appears and the previous document stays open
  - export a sheet and inspect the JSON
- [ ] **Step 3: Run** `ctest --test-dir build --output-on-failure`. Expected: all pass (no core regressions).
- [ ] **Step 4: Commit** `feat(app): main window, canvas, file and edit commands`

---

### Task 10: Layers panel, palette panel, color picker

**Files:**
- Create: `src/app/LayersPanel.h/.cpp`, `src/app/PalettePanel.h/.cpp`, `src/app/ColorPicker.h/.cpp`
- Modify: `src/app/MainWindow.cpp`

**Interfaces:**
- Consumes: `Document::apply` with `addLayer/removeLayer/moveLayer/setLayerProps`, `ToolContext` primary/secondary, `loadPalette`.
- Produces: three `QDockWidget`s. Layers: list top→bottom with eye toggle, rename on double-click, an opacity slider (0–255) and add/delete/up/down buttons; the current layer sets `ctx.layer`. Palette: swatch grid, left click sets primary, right click sets secondary, plus/minus to add the primary color or remove the selected swatch (palette changes go through `apply`). Color picker: an HSV square + hue slider + alpha slider + hex field (`#RRGGBB` or `#RRGGBBAA`) for editing primary/secondary, with the two colors shown as overlapping squares (X swaps them).

Opacity-slider drags must make one undo entry per drag, not one per tick: preview by assigning the sprite directly, then `apply` on release.

- [ ] **Step 1: Implement and dock (left: tools; right: color picker, palette, layers; bottom: timeline in Task 11)**
- [ ] **Step 2: Manual check:**
  - hide a layer and confirm the canvas updates
  - one opacity drag = one undo entry
  - import the DB32 `.gpl` from Lospec
  - type a hex color and draw with it
  - deleting the only layer does nothing
- [ ] **Step 3: Commit** `feat(app): layers, palette and color panels`

---

### Task 11: Timeline, playback, onion skin, tags

**Files:**
- Create: `src/app/Timeline.h/.cpp`
- Modify: `src/app/MainWindow.cpp`, `src/app/CanvasView.cpp`

**Interfaces:**
- Consumes: frame/tag ops from Task 2, `CanvasView::setOnionSkin`, `ToolContext::frame/layer`.
- Produces: `class Timeline : public QWidget` (bottom dock).
  - Grid: columns = frames, rows = layers in the same order as the Layers panel; filled cels show a dot. Clicking a cell sets the current frame and layer.
  - Header shows frame number and duration. Double-clicking it edits the duration (ms); selecting several frames and editing applies the value to all of them.
  - A tag bar above the header draws a colored band per tag with its name. Drag-selecting frame headers and choosing "New Tag" (context menu) asks for a name. Double-clicking a tag renames it or changes its range; the context menu deletes it.
  - Context menu on frames: Add, Duplicate, Delete, Move Left/Right.
  - Toolbar: Play/Pause (Enter), Previous/Next frame (Alt+, / Alt+.), loop toggle, "play selected tag only" toggle, onion toggle + before/after spin boxes 1..3.
  - Playback uses a single-shot `QTimer` re-armed with each frame's `durationMs`. Drawing during playback pauses it.

- [ ] **Step 1: Implement**
- [ ] **Step 2: Manual check:**
  - an 8-frame walk with tag `walk` (frames 1–8) plays at the per-frame durations
  - "tag only" loops just that range
  - onion skin shows faded neighbors
  - deleting frames inside a tag shrinks it
  - playback stays smooth at 256×256 with 10 layers and 100 frames (perf target). If not, profile `CanvasView::paintEvent`: first cache the composite per frame and invalidate it on `changed`.
- [ ] **Step 3: Commit** `feat(app): timeline, playback, onion skin, tags`

---

### Task 12: Autosave, crash recovery, unsaved-changes prompt

**Files:**
- Create: `src/app/Autosave.h/.cpp`
- Modify: `src/app/MainWindow.cpp`, `src/app/main.cpp`

**Interfaces:**
- Consumes: `saveSprx`, `loadSprx`, `Document::isModified`, `Document::filePath`.
- Produces: `class Autosave : public QObject { public: Autosave(MainWindow*); void start(); static QStringList pendingRecoveries(); static void discard(const QString& recoveryPath); };`
  - Every 3 minutes, if the document is modified, it writes `<AppDataLocation>/recovery/<session-uuid>.sprx` plus `<uuid>.json` (`{"originalPath": "...", "savedAt": "<ISO time>"}`).
  - Both files are deleted on a successful save and on a normal close.
  - On startup, `pendingRecoveries()` is non-empty after a crash. A dialog lists each one (original name + time) with Restore / Discard. A restored document opens as modified, with `filePath` set to the original path.
  - Closing the window, New or Open with unsaved changes asks Save / Don't Save / Cancel.

- [ ] **Step 1: Implement**
- [ ] **Step 2: Manual check:**
  - draw, wait for an autosave (temporarily set a 5 s interval), kill the process with `kill -9`, relaunch: the recovery dialog restores the drawing
  - a normal close leaves `recovery/` empty
  - closing with unsaved changes prompts
- [ ] **Step 3: Commit** `feat(app): autosave and crash recovery`

---

### Task 13: Translation readiness check

**Files:**
- Modify: any `src/app/*.cpp` with untranslated strings; `translations/sprixion_en.ts`

- [ ] **Step 1: Run** `cmake --build build --target update_translations`, then `grep -c '<message' translations/sprixion_en.ts`. Expected: count > 0, and `grep -rn 'QString("' src/app` shows no user-visible text.
- [ ] **Step 2: Fake-locale check.** Copy `sprixion_en.ts` to a throwaway `sprixion_xx.ts` with one menu string changed, build it, and run with `LANGUAGE=xx LC_ALL=xx`. Expected: the changed string appears. Delete the throwaway afterwards.
- [ ] **Step 3: Commit** `chore(i18n): all UI strings translatable`

---

### Task 14: Packaging and CI

**Files:**
- Create: `packaging/linux/build-appimage.sh`, `packaging/linux/sprixion.desktop`, `packaging/linux/sprixion.png` (256×256 icon), `packaging/windows/deploy.ps1`, `packaging/windows/sprixion.iss`, `.github/workflows/build.yml`

**Interfaces:**
- Produces:
  - `build-appimage.sh` makes a Release build, then runs `linuxdeploy` + `linuxdeploy-plugin-qt` and writes `Sprixion-0.1.0-x86_64.AppImage`
  - `deploy.ps1` makes a Release build, runs `windeployqt`, zips to `Sprixion-0.1.0-win64.zip`, and runs Inno Setup on `sprixion.iss` to produce `Sprixion-0.1.0-setup.exe`
  - CI builds and runs `ctest` on `ubuntu-22.04` and `windows-2022` with `jurplel/install-qt-action` (Qt 6.5 LTS, the minimum-version guarantee), and uploads both packages as artifacts. *(Plan-time call: CI is the only way to verify the Windows build from this Linux machine.)*

- [ ] **Step 1: Write scripts and workflow**
- [ ] **Step 2: Run** `packaging/linux/build-appimage.sh && ./Sprixion-0.1.0-x86_64.AppImage`. Expected: the app starts.
- [ ] **Step 3: Push to GitHub (only after the user approves creating the remote repo) and check that both CI jobs are green and the artifacts download and run (Windows check by the user)**
- [ ] **Step 4: Commit** `build: AppImage, Windows installer, CI`

---

### Task 15: Unity importer package

**Files:**
- Create: `unity/com.sprixion.importer/package.json`, `README.md`, `LICENSE`, `Editor/Sprixion.Importer.Editor.asmdef`, `Editor/SprxImporter.cs`, `Editor/SprxFile.cs`

**Interfaces:**
- Consumes: the `.sprx` format from Task 6 (only `manifest.json`, `flat/*.png`, `tags`, `frames[].duration`, `sliceGrid`; it ignores layers and cels).
- Produces:
  - `package.json`: `{"name":"com.sprixion.importer","version":"0.1.0","displayName":"Sprixion Importer","unity":"2022.3","description":"Imports Sprixion .sprx files as sprites and animation clips."}`
  - `SprxFile.Read(byte[]) → SprxFile { int Width, Height; List<int> DurationsMs; List<Texture2D> Flats; List<(string Name,int From,int To)> Tags; SliceGrid? Grid }` uses `System.IO.Compression.ZipArchive` + `Texture2D.LoadImage`, and throws `InvalidDataException` with a readable message on bad files or `formatVersion != 1`
  - `[ScriptedImporter(1, "sprx")] class SprxImporter` with inspector fields `float pixelsPerUnit = 16; Vector2 pivot = new(0.5f, 0f); bool loopClips = true;`

Import:
1. Pack the frames into one atlas: columns = ceil(sqrt(frames)), row-major from the top. Texture settings: `filterMode = Point`, no compression, no mipmaps, `wrapMode = Clamp`, `name = "<file>"`. Unity's texture origin is bottom-left, so frame rects use y = `atlasHeight - (row+1)*height`.
2. Without a slice grid: one `Sprite.Create` per frame, named `<file>_<frame>`. With a slice grid: one sprite per cell whose pixels are not all transparent, named `<file>_<frame>_<cellIndex>`, cells walked row-major from the top-left with offset and spacing.
3. Clips, only without a slice grid: one `AnimationClip` per tag, named after the tag. Use `ObjectReferenceKeyframe` on `EditorCurveBinding.PPtrCurve("", typeof(SpriteRenderer), "m_Sprite")`, key times = cumulative durations / 1000, and a duplicate of the last key at the end time so the last frame keeps its duration. `frameRate = 1000`. `loopTime = loopClips` via `AnimationUtility.SetAnimationClipSettings`.
4. `ctx.AddObjectToAsset` for the texture, every sprite and every clip; `ctx.SetMainObject(texture)`. Removed frames and tags disappear on reimport automatically, since ScriptedImporter rebuilds every sub-asset.
5. A bad file calls `ctx.LogImportError(message)` and imports nothing.

- [ ] **Step 1: Implement the package files**
- [ ] **Step 2: Manual check in Unity 2022.3 LTS and Unity 6. Add via Package Manager → "Add package from disk" → `package.json`.**
  - **M1:** Save `walk.sprx` (8 frames, tag `walk` 0–7, durations 100 ms, frame 3 = 200 ms) into `Assets/`. Expected: a texture with 8 crisp, unfiltered sprites and a `walk` clip lasting 0.9 s. Drag the first sprite into the scene, add an Animator with the clip, press Play: it animates.
  - **M2:** Save `tiles.sprx` (64×64, slice grid 16×16, 3 empty cells). Expected: 13 sprites, no clips.
  - **M3:** Delete 4 frames and the tag in Sprixion, re-save over `walk.sprx`. Expected: 4 sprites, no `walk` clip, no stale sub-assets, no console errors.
  - **M4:** Rename a `.txt` to `bad.sprx` in `Assets/`. Expected: an import error in the console, no exception spam.
- [ ] **Step 3: Commit** `feat(unity): .sprx ScriptedImporter package`

---

## Spec coverage map

| Spec item | Task |
|---|---|
| Pencil (pixel-perfect), eraser, fill, line/rect/ellipse, eyedropper, primary/secondary, mirror | 4, 7, 9 |
| Selection, move, copy/cut/paste, flip/rotate (selection/cel/sprite) | 2, 8, 9 |
| Zoom/pan, grid, checkerboard, new sprite 1–2048, resize/crop | 2, 9 |
| Layers (add/delete/reorder/rename/hide/opacity) | 2, 5, 10 |
| Color picker, palette panel, .gpl/.hex import | 8, 10 |
| Timeline, frames, durations, playback, onion skin, tags | 2, 11 |
| Undo/redo everything | 3 + every `apply`/`commitCelEdit` caller |
| .sprx, PNG import/export, sprite sheet + Aseprite-array JSON | 6, 8, 9 |
| Autosave + recovery, atomic saves | 6, 12 |
| Unity importer: texture, sprites, clips, PPU/pivot/loop, slice grid, reimport | 15 |
| i18n readiness | 1, 13 |
| Packaging Windows/Linux | 14 |
