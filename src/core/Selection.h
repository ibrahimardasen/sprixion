#pragma once
#include "Document.h"
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QTransform>

namespace spx {

// Per-document app state, not undoable itself; lifting and dropping pixels is.
struct Selection {
    QRect rect;
    QImage floating;
    QPoint floatingPos;
    bool isActive() const { return !rect.isNull(); }
    bool isFloating() const { return !floating.isNull(); }
};

void liftSelection(Document&, Selection&, int layer, int frame);
void dropSelection(Document&, Selection&, int layer, int frame);
QImage copySelection(const Sprite&, const Selection&, int layer, int frame);
void pasteImage(Document&, Selection&, int layer, int frame, const QImage&, QPoint at);
void transformSelection(Selection&, const QTransform&);

} // namespace spx
