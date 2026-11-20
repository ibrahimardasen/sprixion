#include "EyedropperTool.h"
#include "Compositor.h"

namespace spx {

void EyedropperTool::press(ToolContext& c, QPoint p, Qt::MouseButton b)
{
    if (!isDrawButton(b)) return;
    const QImage img = compositeFrame(c.doc->sprite(), c.frame);
    if (!img.rect().contains(p)) return;
    const QRgb px = img.pixel(p);
    if (qAlpha(px) == 0) return; // an empty pixel would leave an invisible color that draws nothing
    (b == Qt::LeftButton ? c.primary : c.secondary) = QColor::fromRgba(px);
}

} // namespace spx
