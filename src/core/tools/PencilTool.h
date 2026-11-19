#pragma once
#include "Tool.h"
#include <QImage>
#include <QRect>

namespace spx {

class PencilTool : public Tool
{
public:
    explicit PencilTool(bool eraser) : m_eraser(eraser) {}
    void press(ToolContext&, QPoint, Qt::MouseButton) override;
    void move(ToolContext&, QPoint) override;
    void release(ToolContext&, QPoint) override;

private:
    void redraw(ToolContext&);
    bool m_eraser, m_active = false;
    QRgb m_color = 0;
    QImage m_before;
    QVector<QPoint> m_pts;
    QRect m_dirty;
};

} // namespace spx
