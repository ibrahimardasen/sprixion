#include "PencilTool.h"
#include "Raster.h"
#include <QCoreApplication>
#include <QPainter>

namespace spx {

void PencilTool::press(ToolContext& c, QPoint p, Qt::MouseButton b)
{
    if (!isDrawButton(b)) return;
    m_active = true;
    m_color = m_eraser ? 0 : buttonColor(c, b).rgba();
    m_before = c.doc->liveCel(c.layer, c.frame).copy();
    m_pts = {p};
    m_dirty = QRect();
    redraw(c);
}

void PencilTool::move(ToolContext& c, QPoint p)
{
    if (!m_active) return;
    m_pts += raster::line(m_pts.last(), p);
    redraw(c);
}

void PencilTool::release(ToolContext& c, QPoint p)
{
    if (!m_active) return;
    if (p != m_pts.last()) move(c, p);
    m_active = false;
    if (m_dirty.isNull()) return; // fully off-canvas
    const QString text = QCoreApplication::translate("Tool", m_eraser ? "Eraser" : "Pencil");
    c.doc->commitCelEdit(text, c.layer, c.frame, m_dirty, m_before.copy(m_dirty));
}

void PencilTool::redraw(ToolContext& c)
{
    QImage& cel = c.doc->liveCel(c.layer, c.frame);
    if (!m_dirty.isNull()) {
        QPainter qp(&cel);
        qp.setCompositionMode(QPainter::CompositionMode_Source);
        qp.drawImage(m_dirty.topLeft(), m_before, m_dirty);
    }
    const auto pts = raster::mirror(c.pixelPerfect ? raster::pixelPerfect(m_pts) : m_pts, cel.size(), c.mirrorH, c.mirrorV);
    QRect d;
    raster::plot(cel, pts, m_color, &d);
    m_dirty = m_dirty.united(d);
    if (!m_dirty.isNull()) c.doc->touch(m_dirty);
}

} // namespace spx
