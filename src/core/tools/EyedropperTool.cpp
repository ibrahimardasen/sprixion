#include "EyedropperTool.h"
#include "Compositor.h"

namespace spx {

void EyedropperTool::press(ToolContext& c, QPoint p, Qt::MouseButton b)
{
    if (!isDrawButton(b)) return;
    const QImage img = compositeFrame(c.doc->sprite(), c.frame);
    if (!img.rect().contains(p)) return;
    (b == Qt::LeftButton ? c.primary : c.secondary) = QColor::fromRgba(img.pixel(p));
}

} // namespace spx
