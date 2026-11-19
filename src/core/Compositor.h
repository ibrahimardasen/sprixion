#pragma once
#include "Sprite.h"

namespace spx {

// Flattens one frame (visible layers bottom to top, SourceOver, layer opacity applied) to canvas-sized ARGB32.
// Out-of-range frame: transparent image.
QImage compositeFrame(const Sprite &s, int frame);

} // namespace spx
