#pragma once
#include "Tool.h"

namespace spx {

class ShapeTool : public Tool
{
public:
    enum Kind { Line, Rect, Ellipse };
    explicit ShapeTool(Kind k) : m_kind(k) {}
    void press(ToolContext&, QPoint, Qt::MouseButton) override;
    void move(ToolContext&, QPoint) override;
    void release(ToolContext&, QPoint) override;
    QVector<QPoint> preview() const override { return m_pts; }

private:
    void update(ToolContext&, QPoint);
    Kind m_kind;
    bool m_active = false;
    QRgb m_color = 0;
    QPoint m_start;
    QVector<QPoint> m_pts;
};

} // namespace spx
