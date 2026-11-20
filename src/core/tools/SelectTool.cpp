#include "SelectTool.h"

namespace spx {

void SelectTool::press(ToolContext& c, QPoint p, Qt::MouseButton b)
{
    if (b != Qt::LeftButton) return;
    m_start = m_last = p;
    if (m_sel->isActive() && m_sel->rect.contains(p)) {
        liftSelection(*c.doc, *m_sel, c.layer, c.frame);
        m_mode = Moving;
        return;
    }
    dropSelection(*c.doc, *m_sel, c.layer, c.frame);
    m_mode = NewRect;
    move(c, p);
}

void SelectTool::move(ToolContext& c, QPoint p)
{
    const QRect old = m_sel->rect;
    if (m_mode == Moving) {
        m_sel->floatingPos += p - m_last;
        m_sel->rect.moveTopLeft(m_sel->floatingPos);
        m_last = p;
    } else if (m_mode == NewRect) {
        m_sel->rect = QRect(m_start, p).normalized() & QRect(QPoint(), c.doc->sprite().size);
    } else return;
    c.doc->touch(old.united(m_sel->rect));
}

void SelectTool::release(ToolContext& c, QPoint p)
{
    if (m_mode == Idle) return;
    move(c, p);
    if (m_mode == NewRect && p == m_start) {
        const QRect old = m_sel->rect;
        m_sel->rect = QRect();
        c.doc->touch(old);
    }
    m_mode = Idle;
}

} // namespace spx
