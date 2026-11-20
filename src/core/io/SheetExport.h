#pragma once
#include "Sprite.h"
#include <QImage>
#include <QJsonDocument>

namespace spx {

enum class SheetLayout { Horizontal, Grid };
struct SheetResult { QImage image; QJsonDocument json; };

// JSON follows the Aseprite "array" schema so engine importers keep working.
SheetResult exportSheet(const Sprite&, SheetLayout, int columns, const QString& imageFileName);

} // namespace spx
