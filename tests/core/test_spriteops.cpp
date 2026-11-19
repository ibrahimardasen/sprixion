#include "SpriteOps.h"
#include <QtTest>

class TestSpriteOps : public QObject
{
    Q_OBJECT
private slots:
    void makeSpriteDefaults() { auto s = spx::makeSprite({32,32});
      QCOMPARE(s.layers.size(), 1); QCOMPARE(s.layers[0].name, QString("Layer 1")); QCOMPARE(s.frames.size(), 1);
      QCOMPARE(s.frames[0].durationMs, 100); QCOMPARE(s.palette.size(), 16); QVERIFY(s.layers[0].cels[0].isNull()); }
    void cannotRemoveLastLayerOrFrame() { auto s = spx::makeSprite({8,8});
      QCOMPARE(spx::removeLayer(s,0).layers.size(), 1); QCOMPARE(spx::removeFrame(s,0).frames.size(), 1); }
    void removeFrameShrinksAndDropsTags() { auto s = spx::makeSprite({8,8});
      for (int i=0;i<5;++i) s = spx::addFrame(s, 1);
      s = spx::addTag(s, {"walk",1,3}); s = spx::addTag(s, {"hit",5,5});
      s = spx::removeFrame(s, 2); QCOMPARE(s.tags[0].from,1); QCOMPARE(s.tags[0].to,2);
      s = spx::removeFrame(s, 4); QCOMPARE(s.tags.size(), 1); }
    void framesKeepCelsInSync() { auto s = spx::addLayer(spx::makeSprite({8,8}),1);
      s = spx::addFrame(s,1); for (auto& l : s.layers) QCOMPARE(l.cels.size(), 2); }
    void duplicateFrameCopiesPixels() { auto s = spx::makeSprite({4,4});
      spx::celForWrite(s,0,0).setPixel(1,1,qRgba(255,0,0,255)); s = spx::duplicateFrame(s,0);
      QCOMPARE(s.layers[0].cels[1].pixel(1,1), qRgba(255,0,0,255)); }
    void resizeAnchorsCenter() { auto s = spx::makeSprite({4,4}); spx::celForWrite(s,0,0).setPixel(0,0,qRgba(1,2,3,255));
      s = spx::resizeCanvas(s,{8,8},Qt::AlignCenter); QCOMPARE(s.size, QSize(8,8)); QCOMPARE(s.layers[0].cels[0].pixel(2,2), qRgba(1,2,3,255)); }
    void resizeClampsSize() { QCOMPARE(spx::resizeCanvas(spx::makeSprite({4,4}),{5000,0},Qt::AlignCenter).size, QSize(2048,1)); }
    void rotate90SwapsSize() { QCOMPARE(spx::rotateSprite(spx::makeSprite({4,8}),90).size, QSize(8,4)); }
    void cropOutsideCanvasIsClipped() { QCOMPARE(spx::cropCanvas(spx::makeSprite({8,8}),{4,4,10,10}).size, QSize(4,4)); }
    void flipAndTransformCel() { auto s = spx::makeSprite({4,4}); spx::celForWrite(s,0,0).setPixel(0,0,qRgba(9,9,9,255));
      QCOMPARE(spx::flipSprite(s, Qt::Horizontal).layers[0].cels[0].pixel(3,0), qRgba(9,9,9,255));
      auto t = spx::transformCel(s,0,0,QTransform().scale(-1,1));
      QCOMPARE(t.layers[0].cels[0].pixel(3,0), qRgba(9,9,9,255)); QCOMPARE(t.layers[0].cels[0].size(), QSize(4,4)); }
    void outOfRangeIsNoop() { auto s = spx::makeSprite({4,4});
      QCOMPARE(spx::moveFrame(s,5,0).frames.size(), 1); QCOMPARE(spx::setFrameDuration(s,3,5).frames[0].durationMs, 100); }
    void rotate90IsClockwise() { auto s = spx::makeSprite({4,2}); spx::celForWrite(s,0,0).setPixel(0,0,qRgba(255,0,0,255));
      s = spx::rotateSprite(s,90); QCOMPARE(s.size, QSize(2,4));
      QCOMPARE(s.layers[0].cels[0].pixel(1,0), qRgba(255,0,0,255));
      QCOMPARE(s.layers[0].cels[0].format(), QImage::Format_ARGB32);
      QCOMPARE(spx::flipSprite(s, Qt::Vertical).layers[0].cels[0].format(), QImage::Format_ARGB32);
      QCOMPARE(spx::transformCel(s,0,0,QTransform().scale(-1,1)).layers[0].cels[0].format(), QImage::Format_ARGB32); }
    void moveFrameMovesCels() { auto s = spx::makeSprite({4,4}); s = spx::addFrame(s,1);
      spx::celForWrite(s,0,0).setPixel(0,0,qRgba(7,7,7,255)); s = spx::moveFrame(s,0,1);
      QCOMPARE(s.layers[0].cels[1].pixel(0,0), qRgba(7,7,7,255)); QVERIFY(s.layers[0].cels[0].isNull());
      QCOMPARE(s.layers[0].cels.size(), s.frames.size()); }
    void moveLayerMovesNames() { auto s = spx::addLayer(spx::makeSprite({4,4}),1);
      spx::celForWrite(s,0,0).setPixel(0,0,qRgba(7,7,7,255)); s = spx::moveLayer(s,0,1);
      QCOMPARE(s.layers[1].name, QString("Layer 1")); QCOMPARE(s.layers[0].name, QString("Layer 2"));
      QCOMPARE(s.layers[1].cels[0].pixel(0,0), qRgba(7,7,7,255)); QCOMPARE(s.layers[0].cels.size(), s.frames.size()); }
};

QTEST_MAIN(TestSpriteOps)
#include "test_spriteops.moc"
