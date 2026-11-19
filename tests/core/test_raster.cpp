#include "Raster.h"
#include <QtTest>

class TestRaster : public QObject
{
    Q_OBJECT
    static bool symmetric(const QVector<QPoint>& p, QRect r) {
        QSet<QPair<int,int>> s; for (QPoint q : p) s.insert({q.x(), q.y()});
        if (s.size() != p.size()) return false;
        for (QPoint q : p) {
            if (!s.contains({r.left()+r.right()-q.x(), q.y()})) return false;
            if (!s.contains({q.x(), r.top()+r.bottom()-q.y()})) return false;
        }
        return true;
    }
private slots:
    void lineEndpoints() { auto p = spx::raster::line({0,0},{5,2}); QCOMPARE(p.front(),QPoint(0,0)); QCOMPARE(p.back(),QPoint(5,2)); QCOMPARE(p.size(),6); }
    void lineSinglePoint() { QCOMPARE(spx::raster::line({3,3},{3,3}).size(),1); }
    void lineAllOctants() {
        for (QPoint b : {QPoint(5,2),QPoint(2,5),QPoint(-2,5),QPoint(-5,2),QPoint(-5,-2),QPoint(-2,-5),QPoint(2,-5),QPoint(5,-2)}) {
            auto p = spx::raster::line({0,0}, b);
            QCOMPARE(p.front(), QPoint(0,0)); QCOMPARE(p.back(), b);
            QCOMPARE(p.size(), qMax(qAbs(b.x()), qAbs(b.y())) + 1);
        }
    }
    void pixelPerfectRemovesCorner() { QVector<QPoint> s{{0,0},{1,0},{1,1}}; QCOMPARE(spx::raster::pixelPerfect(s), (QVector<QPoint>{{0,0},{1,1}})); }
    void pixelPerfectKeepsStraight() { QVector<QPoint> s{{0,0},{1,0},{2,0}}; QCOMPARE(spx::raster::pixelPerfect(s), s); }
    void pixelPerfectDedupsJoints() {
        QVector<QPoint> s{{0,0},{1,0},{1,0},{2,0}};
        QCOMPARE(spx::raster::pixelPerfect(s), (QVector<QPoint>{{0,0},{1,0},{2,0}}));
    }
    void ellipseOutline5x5() { auto p = spx::raster::ellipse({0,0,5,5},false);
      for (QPoint q : {QPoint(2,0),QPoint(0,2),QPoint(4,2),QPoint(2,4)}) QVERIFY(p.contains(q)); QVERIFY(!p.contains(QPoint(0,0))); QVERIFY(!p.contains(QPoint(2,2))); }
    void ellipseFilledHasCenter() { QVERIFY(spx::raster::ellipse({0,0,5,5},true).contains(QPoint(2,2))); }
    void ellipse1x1() { QCOMPARE(spx::raster::ellipse({0,0,1,1},false).size(),1); }
    void ellipseSymmetric() {
        QRect r(3,2,6,4);
        QVERIFY(symmetric(spx::raster::ellipse(r,false), r));
        QVERIFY(symmetric(spx::raster::ellipse(r,true), r));
    }
    void rectFilledCount() { QCOMPARE(spx::raster::rect({0,0,3,2},true).size(),6); QCOMPARE(spx::raster::rect({0,0,3,3},false).size(),8); }
    void rectThin() { QCOMPARE(spx::raster::rect({0,0,1,4},false).size(),4); QCOMPARE(spx::raster::rect({0,0,4,1},false).size(),4); }
    void mirrorBoth() { auto p = spx::raster::mirror({{0,0}},{4,4},true,true); QCOMPARE(p.size(),4); QVERIFY(p.contains(QPoint(3,3))); }
    void mirrorDedups() { QCOMPARE(spx::raster::mirror({{1,1},{2,1}},{4,4},true,false).size(),2); }
    void plotClipsOutside() { QImage i(4,4,QImage::Format_ARGB32); i.fill(0); QRect d;
      spx::raster::plot(i,{{-1,0},{10,10},{1,1}},qRgba(255,0,0,255),&d); QCOMPARE(d,QRect(1,1,1,1)); }
    void plotNullDirtyAndNoWrite() { QImage i(4,4,QImage::Format_ARGB32); i.fill(0);
      spx::raster::plot(i,{{1,1}},qRgba(1,2,3,4),nullptr); QCOMPARE(i.pixel(1,1),qRgba(1,2,3,4));
      QRect d(7,7,1,1); spx::raster::plot(i,{{-1,-1}},1,&d); QCOMPARE(d,QRect(7,7,1,1)); }
    void fillStopsAtBorder() { QImage i(5,5,QImage::Format_ARGB32); i.fill(0);
      for (int y=0;y<5;++y) i.setPixel(2,y,qRgba(0,0,0,255)); QRect d;
      spx::raster::floodFill(i,{0,0},qRgba(255,0,0,255),&d); QCOMPARE(i.pixel(1,4),qRgba(255,0,0,255)); QCOMPARE(qAlpha(i.pixel(3,0)),0);
      QCOMPARE(d,QRect(0,0,2,5)); }
    void fillNoOps() { QImage i(3,3,QImage::Format_ARGB32); i.fill(0); QRect d(9,9,1,1);
      spx::raster::floodFill(i,{5,5},1,&d); spx::raster::floodFill(i,{0,0},0,&d); QCOMPARE(d,QRect(9,9,1,1)); QCOMPARE(i.pixel(0,0),QRgb(0)); }
    void fillLargeCanvasNoStackOverflow() { QImage i(2048,2048,QImage::Format_ARGB32); i.fill(0); QRect d;
      spx::raster::floodFill(i,{0,0},qRgba(1,1,1,255),&d); QCOMPARE(d,QRect(0,0,2048,2048)); }
};

QTEST_MAIN(TestRaster)
#include "test_raster.moc"
