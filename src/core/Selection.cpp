#include "Selection.h"
#include <QCoreApplication>
#include <QPainter>

namespace spx {

void liftSelection(Document& d, Selection& s, int layer, int frame)
{
    if (s.isFloating()) return;
    const QRect r = s.rect & QRect(QPoint(), d.sprite().size);
    if (r.isEmpty()) return;
    QImage& cel = d.liveCel(layer, frame);
    const QImage before = cel.copy(r);
    s.floating = before.convertToFormat(QImage::Format_ARGB32);
    s.floatingPos = r.topLeft();
    s.rect = r;
    QPainter p(&cel);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(r, Qt::transparent);
    p.end();
    d.commitCelEdit(QCoreApplication::translate("Tool", "Move Selection"), layer, frame, r, before);
    d.touch(r);
}

void dropSelection(Document& d, Selection& s, int layer, int frame)
{
    if (!s.isFloating()) return;
    const QRect r = QRect(s.floatingPos, s.floating.size()) & QRect(QPoint(), d.sprite().size);
    if (!r.isEmpty()) {
        QImage& cel = d.liveCel(layer, frame);
        const QImage before = cel.copy(r);
        QPainter p(&cel);
        p.drawImage(s.floatingPos, s.floating); // SourceOver; painter clips to the cel
        p.end();
        d.commitCelEdit(QCoreApplication::translate("Tool", "Drop Selection"), layer, frame, r, before);
        d.touch(r);
    }
    s.floating = QImage();
    s.rect = r;
}

QImage copySelection(const Sprite& sp, const Selection& s, int layer, int frame)
{
    if (s.isFloating()) return s.floating;
    const QRect r = s.rect & QRect(QPoint(), sp.size);
    if (r.isEmpty()) return {};
    if (layer < 0 || layer >= sp.layers.size() || frame < 0 || frame >= sp.layers[layer].cels.size()
        || sp.layers[layer].cels[frame].isNull()) {
        QImage blank(r.size(), QImage::Format_ARGB32);
        blank.fill(Qt::transparent);
        return blank;
    }
    return sp.layers[layer].cels[frame].copy(r);
}

void pasteImage(Document& d, Selection& s, int layer, int frame, const QImage& img, QPoint at)
{
    if (img.isNull()) return;
    dropSelection(d, s, layer, frame);
    s.floating = img.convertToFormat(QImage::Format_ARGB32);
    s.floatingPos = at;
    s.rect = QRect(at, img.size());
}

void transformSelection(Selection& s, const QTransform& t)
{
    if (!s.isFloating()) return;
    s.floating = s.floating.transformed(t); // flips/90° turns only; no smoothing
    s.rect = QRect(s.floatingPos, s.floating.size());
}

} // namespace spx
