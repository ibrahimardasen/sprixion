#include "SpriteOps.h"
#include <QPainter>
#include <algorithm>

namespace spx {

static bool okL(const Sprite &s, int i) { return i >= 0 && i < s.layers.size(); }
static bool okF(const Sprite &s, int i) { return i >= 0 && i < s.frames.size(); }

template <class F> static void forCels(Sprite &s, F f)
{
    for (Layer &l : s.layers)
        for (QImage &c : l.cels)
            if (!c.isNull()) c = f(c);
}

Sprite addLayer(Sprite s, int index)
{
    if (index < 0 || index > s.layers.size()) return s;
    Layer l;
    l.name = QStringLiteral("Layer %1").arg(s.layers.size() + 1);
    l.cels.resize(s.frames.size());
    s.layers.insert(index, l);
    return s;
}

Sprite removeLayer(Sprite s, int index)
{
    if (!okL(s, index) || s.layers.size() <= 1) return s;
    s.layers.remove(index);
    return s;
}

Sprite moveLayer(Sprite s, int from, int to)
{
    if (!okL(s, from) || !okL(s, to)) return s;
    s.layers.move(from, to);
    return s;
}

Sprite setLayerProps(Sprite s, int index, QString name, bool visible, int opacity)
{
    if (!okL(s, index)) return s;
    Layer &l = s.layers[index];
    l.name = name;
    l.visible = visible;
    l.opacity = std::clamp(opacity, 0, 255);
    return s;
}

Sprite addFrame(Sprite s, int index)
{
    if (index < 0 || index > s.frames.size()) return s;
    s.frames.insert(index, Frame{});
    for (Layer &l : s.layers) l.cels.insert(index, QImage());
    return s;
}

Sprite duplicateFrame(Sprite s, int index)
{
    if (!okF(s, index)) return s;
    s.frames.insert(index + 1, s.frames[index]);
    for (Layer &l : s.layers) l.cels.insert(index + 1, l.cels[index]);
    return s;
}

Sprite removeFrame(Sprite s, int index)
{
    if (!okF(s, index) || s.frames.size() <= 1) return s;
    s.frames.remove(index);
    for (Layer &l : s.layers) l.cels.remove(index);
    QVector<Tag> out;
    for (Tag t : std::as_const(s.tags)) {
        if (index > t.to) {
        } else if (index < t.from) {
            --t.from; --t.to;
        } else if (--t.to < t.from) {
            continue;
        }
        out.append(t);
    }
    s.tags = out;
    return s;
}

Sprite moveFrame(Sprite s, int from, int to)
{
    if (!okF(s, from) || !okF(s, to)) return s;
    s.frames.move(from, to);
    for (Layer &l : s.layers) l.cels.move(from, to);
    return s;
}

Sprite setFrameDuration(Sprite s, int index, int ms)
{
    if (!okF(s, index)) return s;
    s.frames[index].durationMs = std::clamp(ms, 1, 65535);
    return s;
}

Sprite addTag(Sprite s, Tag t) { s.tags.append(t); return s; }

Sprite removeTag(Sprite s, int index)
{
    if (index >= 0 && index < s.tags.size()) s.tags.remove(index);
    return s;
}

Sprite setTag(Sprite s, int index, Tag t)
{
    if (index >= 0 && index < s.tags.size()) s.tags[index] = t;
    return s;
}

Sprite resizeCanvas(Sprite s, QSize size, Qt::Alignment anchor)
{
    size = {std::clamp(size.width(), 1, 2048), std::clamp(size.height(), 1, 2048)};
    const int dw = size.width() - s.size.width(), dh = size.height() - s.size.height();
    const int dx = (anchor & Qt::AlignLeft) ? 0 : (anchor & Qt::AlignRight) ? dw : dw / 2;
    const int dy = (anchor & Qt::AlignTop) ? 0 : (anchor & Qt::AlignBottom) ? dh : dh / 2;
    forCels(s, [&](const QImage &c) {
        QImage n(size, QImage::Format_ARGB32);
        n.fill(Qt::transparent);
        QPainter p(&n);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawImage(dx, dy, c);
        return n;
    });
    s.size = size;
    return s;
}

Sprite cropCanvas(Sprite s, QRect r)
{
    r &= QRect(QPoint(0, 0), s.size);
    if (r.isEmpty()) return s;
    forCels(s, [&](const QImage &c) { return c.copy(r); });
    s.size = r.size();
    return s;
}

Sprite flipSprite(Sprite s, Qt::Orientation o)
{
    const bool h = o == Qt::Horizontal;
    forCels(s, [&](const QImage &c) { return c.mirrored(h, !h); });
    return s;
}

Sprite rotateSprite(Sprite s, int degrees)
{
    degrees = ((degrees % 360) + 360) % 360;
    if (degrees != 90 && degrees != 180 && degrees != 270) return s;
    forCels(s, [&](const QImage &c) { return c.transformed(QTransform().rotate(degrees)); });
    if (degrees != 180) s.size.transpose();
    return s;
}

Sprite transformCel(Sprite s, int layer, int frame, const QTransform &t)
{
    if (!okL(s, layer) || !okF(s, frame) || s.layers[layer].cels[frame].isNull()) return s;
    const QImage src = s.layers[layer].cels[frame];
    const QPointF c(s.size.width() / 2.0, s.size.height() / 2.0);
    QImage n(s.size, QImage::Format_ARGB32);
    n.fill(Qt::transparent);
    QPainter p(&n);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.setTransform(QTransform().translate(-c.x(), -c.y()) * t * QTransform().translate(c.x(), c.y()));
    p.drawImage(0, 0, src);
    p.end();
    s.layers[layer].cels[frame] = n;
    return s;
}

} // namespace spx
