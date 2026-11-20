#include "ShapeTool.h"
#include "Raster.h"
#include <QCoreApplication>
#include <QtGlobal>

namespace spx {

void ShapeTool::press(ToolContext& c, QPoint p, Qt::MouseButton b)
{
    if (!isDrawButton(b)) return;
    m_active = true;
    m_start = p;
    m_color = buttonColor(c, b).rgba();
    update(c, p);
}

void ShapeTool::move(ToolContext& c, QPoint p)
{
    if (m_active) update(c, p);
}

void ShapeTool::update(ToolContext& c, QPoint p)
{
    if (m_kind == Line) {
        m_pts = raster::line(m_start, p);
    } else {
        if (c.constrain) {
            const int s = qMax(qAbs(p.x() - m_start.x()), qAbs(p.y() - m_start.y()));
            p = m_start + QPoint(p.x() < m_start.x() ? -s : s, p.y() < m_start.y() ? -s : s);
        }
        const QRect r = QRect(m_start, p).normalized();
        m_pts = m_kind == Rect ? raster::rect(r, c.filled) : raster::ellipse(r, c.filled);
    }
    m_pts = raster::mirror(m_pts, c.doc->sprite().size, c.mirrorH, c.mirrorV);
}

void ShapeTool::release(ToolContext& c, QPoint p)
{
    if (!m_active) return;
    update(c, p);
    m_active = false;
    QImage& cel = c.doc->liveCel(c.layer, c.frame);
    const QImage before = cel.copy();
    QRect d;
    raster::plot(cel, m_pts, m_color, &d);
    m_pts.clear();
    if (d.isNull()) return;
    c.doc->touch(d);
    const char* name = m_kind == Line   ? QT_TRANSLATE_NOOP("Tool", "Line")
                       : m_kind == Rect ? QT_TRANSLATE_NOOP("Tool", "Rectangle")
                                        : QT_TRANSLATE_NOOP("Tool", "Ellipse");
    c.doc->commitCelEdit(QCoreApplication::translate("Tool", name), c.layer, c.frame, d, before.copy(d));
}

} // namespace spx
