#pragma once
#include <QColor>
#include <QImage>
#include <QSize>
#include <QString>
#include <QVector>
#include <optional>

namespace spx {

struct Layer {
    QString name;
    bool visible = true;
    int opacity = 255;
    QVector<QImage> cels; // size == frames.size(); null QImage = empty cel
};
struct Frame { int durationMs = 100; };
struct Tag { QString name; int from = 0; int to = 0; }; // 0-based, inclusive
struct SliceGrid { int cellWidth = 16, cellHeight = 16, offsetX = 0, offsetY = 0, spacingX = 0, spacingY = 0; };

struct Sprite {
    QSize size;
    QVector<Layer> layers; // [0] = bottom
    QVector<Frame> frames;
    QVector<Tag> tags;
    QVector<QColor> palette;
    std::optional<SliceGrid> sliceGrid;
};

Sprite makeSprite(QSize size);
// Creates a transparent ARGB32 canvas-sized cel if null.
QImage& celForWrite(Sprite &s, int layer, int frame);

} // namespace spx
