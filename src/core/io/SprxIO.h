#pragma once
#include "Sprite.h"

namespace spx {

// .sprx = zip of manifest.json + cels/<layer>_<frame>.png + flat/<frame>.png (derived, write-only). `error` may be nullptr.
bool saveSprx(const Sprite &, const QString &path, QString *error);
std::optional<Sprite> loadSprx(const QString &path, QString *error);

} // namespace spx
