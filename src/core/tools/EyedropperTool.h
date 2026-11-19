#pragma once
#include "Tool.h"

namespace spx {

class EyedropperTool : public Tool
{
public:
    void press(ToolContext&, QPoint, Qt::MouseButton) override;
    void move(ToolContext&, QPoint) override {}
    void release(ToolContext&, QPoint) override {}
};

} // namespace spx
