#include "Sprite.h"

namespace spx {

Sprite makeSprite(QSize size)
{
    Sprite s;
    s.size = size;
    Layer l;
    l.name = QStringLiteral("Layer 1");
    l.cels.resize(1);
    s.layers.append(l);
    s.frames.append(Frame{});
    for (const char *c : {"140c1c", "442434", "30346d", "4e4a4e", "854c30", "346524", "d04648", "757161",
                          "597dce", "d27d2c", "8595a1", "6daa2c", "d2aa99", "6dc2ca", "dad45e", "deeed6"})
        s.palette.append(QColor(QStringLiteral("#") + QLatin1String(c)));
    return s;
}

QImage& celForWrite(Sprite &s, int layer, int frame)
{
    QImage &c = s.layers[layer].cels[frame];
    if (c.isNull()) {
        c = QImage(s.size, QImage::Format_ARGB32);
        c.fill(Qt::transparent);
    }
    return c;
}

} // namespace spx
