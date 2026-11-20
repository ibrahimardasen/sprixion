#include "SheetExport.h"
#include "Compositor.h"
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>

namespace spx {

SheetResult exportSheet(const Sprite& s, SheetLayout layout, int columns, const QString& imageFileName)
{
    const int n = s.frames.size(), w = s.size.width(), h = s.size.height();
    const int cols = layout == SheetLayout::Horizontal ? qMax(1, n) : qBound(1, columns, qMax(1, n));
    const int rows = (n + cols - 1) / cols;
    QImage img(cols * w, rows * h, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    const QString base = QFileInfo(imageFileName).completeBaseName();
    QJsonArray frames;
    for (int i = 0; i < n; ++i) {
        const int x = (i % cols) * w, y = (i / cols) * h;
        p.drawImage(x, y, compositeFrame(s, i));
        const QJsonObject rect{{"x", 0}, {"y", 0}, {"w", w}, {"h", h}};
        frames << QJsonObject{{"filename", QString("%1 %2.png").arg(base).arg(i)},
                              {"frame", QJsonObject{{"x", x}, {"y", y}, {"w", w}, {"h", h}}},
                              {"rotated", false}, {"trimmed", false}, {"spriteSourceSize", rect},
                              {"sourceSize", QJsonObject{{"w", w}, {"h", h}}}, {"duration", s.frames[i].durationMs}};
    }
    p.end();
    QJsonArray tags;
    for (const Tag& t : s.tags)
        tags << QJsonObject{{"name", t.name}, {"from", t.from}, {"to", t.to}, {"direction", "forward"}};
    const QJsonObject meta{{"app", "Sprixion"}, {"version", "0.1.1"}, {"image", imageFileName}, {"format", "RGBA8888"},
                           {"size", QJsonObject{{"w", img.width()}, {"h", img.height()}}}, {"scale", "1"}, {"frameTags", tags}};
    return {img, QJsonDocument(QJsonObject{{"frames", frames}, {"meta", meta}})};
}

} // namespace spx
