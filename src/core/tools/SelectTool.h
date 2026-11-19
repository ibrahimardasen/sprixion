#pragma once
#include "Selection.h"
#include "Tool.h"

namespace spx {

class SelectTool : public Tool
{
public:
    explicit SelectTool(Selection* s) : m_sel(s) {}
    void press(ToolContext&, QPoint, Qt::MouseButton) override;
    void move(ToolContext&, QPoint) override;
    void release(ToolContext&, QPoint) override;

private:
    enum Mode { Idle, NewRect, Moving };
    Selection* m_sel;
    Mode m_mode = Idle;
    QPoint m_start, m_last;
};

} // namespace spx
