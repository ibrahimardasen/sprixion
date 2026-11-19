#include "FillTool.h"
#include "Raster.h"
#include <QCoreApplication>

namespace spx {

void FillTool::press(ToolContext& c, QPoint p, Qt::MouseButton b)
{
    if (!isDrawButton(b)) return;
    QImage& cel = c.doc->liveCel(c.layer, c.frame);
    const QImage before = cel.copy();
    QRect d;
    raster::floodFill(cel, p, buttonColor(c, b).rgba(), &d);
    if (d.isNull()) return;
    c.doc->touch(d);
    c.doc->commitCelEdit(QCoreApplication::translate("Tool", "Fill"), c.layer, c.frame, d, before.copy(d));
}

} // namespace spx
