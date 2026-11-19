#pragma once
#include <QColor>
#include <QString>
#include <QVector>
#include <optional>

namespace spx {

// By extension: .gpl / .hex (case-insensitive). On failure returns nullopt and fills *error (may be null).
std::optional<QVector<QColor>> loadPalette(const QString& path, QString* error);

} // namespace spx
