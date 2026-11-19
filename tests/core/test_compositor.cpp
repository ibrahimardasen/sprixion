#include "Compositor.h"
#include "SpriteOps.h"
#include <QtTest>

class TestCompositor : public QObject
{
    Q_OBJECT
private slots:
    void hiddenLayerIgnored() { auto s = spx::addLayer(spx::makeSprite({2,2}),1);
      spx::celForWrite(s,1,0).fill(qRgba(255,0,0,255)); s.layers[1].visible = false;
      QCOMPARE(qAlpha(spx::compositeFrame(s,0).pixel(0,0)), 0); }
    void opacityApplied() { auto s = spx::makeSprite({1,1}); spx::celForWrite(s,0,0).fill(qRgba(255,0,0,255)); s.layers[0].opacity = 128;
      QVERIFY(qAbs(qAlpha(spx::compositeFrame(s,0).pixel(0,0)) - 128) <= 1); }
    void topLayerWins() { auto s = spx::addLayer(spx::makeSprite({1,1}),1);
      spx::celForWrite(s,0,0).fill(qRgba(255,0,0,255)); spx::celForWrite(s,1,0).fill(qRgba(0,0,255,255));
      QCOMPARE(spx::compositeFrame(s,0).pixel(0,0), qRgba(0,0,255,255)); }
    void semiTransparentOverOpaque() { auto s = spx::addLayer(spx::makeSprite({1,1}),1);
      spx::celForWrite(s,0,0).fill(qRgba(255,0,0,255)); spx::celForWrite(s,1,0).fill(qRgba(0,0,255,128));
      QRgb p = spx::compositeFrame(s,0).pixel(0,0);
      QCOMPARE(qAlpha(p), 255);
      QVERIFY(qAbs(qRed(p) - 127) <= 2); QVERIFY(qAbs(qBlue(p) - 128) <= 2); }
    void badFrameIsTransparent() { auto s = spx::makeSprite({3,2});
      QImage r = spx::compositeFrame(s,99);
      QCOMPARE(r.size(), QSize(3,2)); QCOMPARE(qAlpha(r.pixel(0,0)), 0); }
};

QTEST_MAIN(TestCompositor)
#include "test_compositor.moc"
