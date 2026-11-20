#include "Compositor.h"
#include <QPainter>

namespace spx {

QImage compositeFrame(const Sprite &s, int frame)
{
    // shortcut: premultiplied compositing drifts low-alpha colours; switch to straight-alpha blending if semi-transparent art matters.
    QImage out(s.size, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    if (frame >= 0 && frame < s.frames.size()) {
        QPainter p(&out);
        for (const Layer &l : s.layers) {
            if (!l.visible || l.opacity <= 0 || l.cels[frame].isNull()) continue;
            p.setOpacity(l.opacity / 255.0);
            p.drawImage(0, 0, l.cels[frame]);
        }
    }
    return out.convertToFormat(QImage::Format_ARGB32);
}

} // namespace spx
