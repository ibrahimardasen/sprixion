#pragma once
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QVector>

namespace spx::raster {

QVector<QPoint> line(QPoint a, QPoint b);                     // Bresenham, includes both ends
QVector<QPoint> pixelPerfect(const QVector<QPoint>& stroke);  // drops L-corner pixels; dedups consecutive equal points
QVector<QPoint> rect(QRect r, bool filled);
QVector<QPoint> ellipse(QRect r, bool filled);                // inscribed in r, symmetric
QVector<QPoint> mirror(const QVector<QPoint>&, QSize canvas, bool h, bool v);
void plot(QImage& img, const QVector<QPoint>&, QRgb color, QRect* dirty);
void floodFill(QImage& img, QPoint seed, QRgb color, QRect* dirty);

} // namespace spx::raster
