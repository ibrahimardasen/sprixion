#include "Raster.h"
#include <QSet>
#include <QtGlobal>
#include <algorithm>

namespace spx::raster {

QVector<QPoint> line(QPoint a, QPoint b)
{
    QVector<QPoint> out;
    const int dx = qAbs(b.x() - a.x()), dy = -qAbs(b.y() - a.y());
    const int sx = a.x() < b.x() ? 1 : -1, sy = a.y() < b.y() ? 1 : -1;
    int err = dx + dy, x = a.x(), y = a.y();
    for (;;) {
        out.append({x, y});
        if (x == b.x() && y == b.y()) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
    return out;
}

QVector<QPoint> pixelPerfect(const QVector<QPoint>& stroke)
{
    QVector<QPoint> p;
    for (QPoint q : stroke) if (p.isEmpty() || p.back() != q) p.append(q);
    if (p.size() < 3) return p;
    QVector<QPoint> out{p[0]};
    for (int i = 1; i + 1 < p.size(); ++i) {
        const QPoint prev = out.back(), cur = p[i], next = p[i + 1];
        const bool corner = qAbs(prev.x() - next.x()) == 1 && qAbs(prev.y() - next.y()) == 1
            && (cur.x() == prev.x() || cur.y() == prev.y())
            && (cur.x() == next.x() || cur.y() == next.y());
        if (!corner) out.append(cur);
    }
    out.append(p.back());
    return out;
}

QVector<QPoint> rect(QRect r, bool filled)
{
    r = r.normalized();
    QVector<QPoint> out;
    for (int y = r.top(); y <= r.bottom(); ++y)
        for (int x = r.left(); x <= r.right(); ++x)
            if (filled || x == r.left() || x == r.right() || y == r.top() || y == r.bottom())
                out.append({x, y});
    return out;
}

QVector<QPoint> ellipse(QRect r, bool filled)
{
    r = r.normalized();
    const qint64 w = r.width(), h = r.height();
    // Pixel centre inside the ellipse with semi-axes w/2, h/2, in doubled integer coordinates (exact, symmetric).
    auto inside = [&](qint64 x, qint64 y) {
        if (x < 0 || y < 0 || x >= w || y >= h) return false;
        const qint64 dx = 2 * x - (w - 1), dy = 2 * y - (h - 1);
        return dx * dx * h * h + dy * dy * w * w <= w * w * h * h;
    };
    QVector<QPoint> out;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (inside(x, y) && (filled || !inside(x - 1, y) || !inside(x + 1, y) || !inside(x, y - 1) || !inside(x, y + 1)))
                out.append({r.left() + x, r.top() + y});
    return out;
}

QVector<QPoint> mirror(const QVector<QPoint>& pts, QSize canvas, bool h, bool v)
{
    QVector<QPoint> out;
    QSet<QPoint> seen;
    auto add = [&](QPoint p) {
        if (!seen.contains(p)) { seen.insert(p); out.append(p); }
    };
    for (QPoint p : pts) {
        add(p);
        if (h) add({canvas.width() - 1 - p.x(), p.y()});
        if (v) add({p.x(), canvas.height() - 1 - p.y()});
        if (h && v) add({canvas.width() - 1 - p.x(), canvas.height() - 1 - p.y()});
    }
    return out;
}

void plot(QImage& img, const QVector<QPoint>& pts, QRgb color, QRect* dirty)
{
    QRect box;
    for (QPoint p : pts) {
        if (!img.rect().contains(p)) continue;
        img.setPixel(p, color);
        box = box.isNull() ? QRect(p, QSize(1, 1)) : box.united(QRect(p, QSize(1, 1)));
    }
    if (dirty && !box.isNull()) *dirty = box;
}

void floodFill(QImage& img, QPoint seed, QRgb color, QRect* dirty)
{
    if (!img.rect().contains(seed)) return;
    const QRgb target = img.pixel(seed);
    if (target == color) return;
    const int W = img.width(), H = img.height();
    int minX = W, maxX = -1, minY = H, maxY = -1;
    QVector<QPoint> stack{seed};
    while (!stack.isEmpty()) {
        const QPoint s = stack.takeLast();
        QRgb* row = reinterpret_cast<QRgb*>(img.scanLine(s.y()));
        if (row[s.x()] != target) continue;
        int l = s.x(), r = s.x();
        while (l > 0 && row[l - 1] == target) --l;
        while (r < W - 1 && row[r + 1] == target) ++r;
        for (int x = l; x <= r; ++x) row[x] = color;
        minX = std::min(minX, l); maxX = std::max(maxX, r);
        minY = std::min(minY, s.y()); maxY = std::max(maxY, s.y());
        for (int ny : {s.y() - 1, s.y() + 1}) {
            if (ny < 0 || ny >= H) continue;
            const QRgb* nrow = reinterpret_cast<const QRgb*>(img.constScanLine(ny));
            bool inRun = false;
            for (int x = l; x <= r; ++x) {
                const bool m = nrow[x] == target;
                if (m && !inRun) stack.append({x, ny});
                inRun = m;
            }
        }
    }
    if (dirty) *dirty = QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

} // namespace spx::raster
