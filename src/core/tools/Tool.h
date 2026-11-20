#pragma once
#include "Document.h"
#include <QColor>
#include <QPoint>
#include <QVector>

namespace spx {

struct ToolContext {
    Document* doc;
    int layer = 0;
    int frame = 0;
    QColor primary{Qt::black}, secondary{Qt::white};
    bool mirrorH = false, mirrorV = false;
    bool pixelPerfect = true;
    bool filled = false;
    bool constrain = false; // Shift held: rect -> square, ellipse -> circle
};

class Tool
{
public:
    virtual ~Tool() = default;
    virtual void press(ToolContext&, QPoint canvasPos, Qt::MouseButton) = 0;
    virtual void move(ToolContext&, QPoint canvasPos) = 0;
    virtual void release(ToolContext&, QPoint canvasPos) = 0;
    virtual QVector<QPoint> preview() const { return {}; } // shapes: overlay points while dragging
};

inline bool isDrawButton(Qt::MouseButton b) { return b == Qt::LeftButton || b == Qt::RightButton; }
inline QColor buttonColor(const ToolContext& c, Qt::MouseButton b) { return b == Qt::LeftButton ? c.primary : c.secondary; }

} // namespace spx
