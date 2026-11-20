#!/usr/bin/env bash
# Run on an old distro (CI uses ubuntu-22.04): an AppImage only runs on glibc at least as new as the build host's.
# On rolling distros also set NO_STRIP=1 (linuxdeploy's bundled strip rejects new .relr sections).
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
tools=$root/.cache/linuxdeploy
cd "$root"
mkdir -p "$tools"
base=https://github.com/linuxdeploy
for f in linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
         linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage; do
  [ -x "$tools/$(basename "$f")" ] || { curl -fL -o "$tools/$(basename "$f")" "$base/$f"; chmod +x "$tools/$(basename "$f")"; }
done

cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
rm -rf build-release/AppDir

export PATH="$tools:$PATH"
export APPIMAGE_EXTRACT_AND_RUN=1 # no FUSE needed (CI, containers)
export QMAKE=${QMAKE:-$(command -v qmake6 || command -v qmake)}
export EXTRA_QT_PLUGINS="iconengines;imageformats" # SVG icons are loaded at runtime, so make sure they ship
export EXTRA_PLATFORM_PLUGINS=libqoffscreen.so # lets CI smoke-test headless
export LINUXDEPLOY_OUTPUT_VERSION=0.1.0
linuxdeploy-x86_64.AppImage --appdir build-release/AppDir --plugin qt \
  --executable build-release/src/app/sprixion \
  --desktop-file packaging/linux/sprixion.desktop \
  --icon-file packaging/linux/sprixion.png \
  --output appimage
