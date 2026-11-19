#include "Document.h"
#include "SpriteOps.h"
#include <QtTest>
using namespace spx;

class TestDocument : public QObject
{
    Q_OBJECT
private slots:
    void applyUndoRedo() { Document d(makeSprite({8,8}));
      d.apply("Add Layer", addLayer(d.sprite(),1)); QCOMPARE(d.sprite().layers.size(),2);
      d.undoStack()->undo(); QCOMPARE(d.sprite().layers.size(),1); d.undoStack()->redo(); QCOMPARE(d.sprite().layers.size(),2); }
    void celEditUndoRestoresPixels() { Document d(makeSprite({8,8})); QRect r(2,2,1,1);
      QImage before = d.liveCel(0,0).copy(r); d.liveCel(0,0).setPixel(2,2,qRgba(255,0,0,255));
      d.commitCelEdit("Pencil",0,0,r,before); QCOMPARE(d.sprite().layers[0].cels[0].pixel(2,2), qRgba(255,0,0,255));
      d.undoStack()->undo(); QCOMPARE(qAlpha(d.sprite().layers[0].cels[0].pixel(2,2)), 0);
      d.undoStack()->redo(); QCOMPARE(d.sprite().layers[0].cels[0].pixel(2,2), qRgba(255,0,0,255)); }
    void modifiedTracksCleanState() { Document d(makeSprite({8,8})); QVERIFY(!d.isModified());
      d.apply("x", addFrame(d.sprite(),1)); QVERIFY(d.isModified()); d.undoStack()->setClean(); QVERIFY(!d.isModified()); }
    void changedSignalFires() { Document d(makeSprite({8,8})); QSignalSpy spy(&d, &Document::changed);
      d.apply("x", addFrame(d.sprite(),1)); QCOMPARE(spy.count(),1); }
    void previewSpriteHasNoUndoAndEmitsChanged() { Document d(makeSprite({8,8})); QSignalSpy spy(&d, &Document::changed);
      d.previewSprite(addLayer(d.sprite(),1)); QCOMPARE(d.sprite().layers.size(),2);
      QCOMPARE(spy.count(),1); QVERIFY(spy.takeFirst().at(0).toRect().isNull()); QVERIFY(!d.undoStack()->canUndo()); }
    void touchAndCelUndoEmitDirtyRect() { Document d(makeSprite({8,8})); QSignalSpy spy(&d, &Document::changed);
      d.touch(QRect(1,1,2,2)); QCOMPARE(spy.takeFirst().at(0).toRect(), QRect(1,1,2,2));
      QRect r(2,2,1,1); QImage b = d.liveCel(0,0).copy(r); d.liveCel(0,0).setPixel(2,2,qRgba(1,2,3,255));
      d.commitCelEdit("P",0,0,r,b); QCOMPARE(spy.count(),0);
      d.undoStack()->undo(); QCOMPARE(spy.takeFirst().at(0).toRect(), r); }
    void undoRestoresPartialAlphaExactly() { Document d(makeSprite({8,8})); QRect r(3,3,1,1);
      auto stroke = [&](QRgb c) { QImage b = d.liveCel(0,0).copy(r); d.liveCel(0,0).setPixel(3,3,c); d.commitCelEdit("P",0,0,r,b); };
      auto px = [&] { return d.sprite().layers[0].cels[0].pixel(3,3); };
      const QRgb a = qRgba(10,200,30,77), b = qRgba(250,5,99,201);
      stroke(a); stroke(b); d.undoStack()->undo(); QCOMPARE(px(), a);
      d.undoStack()->redo(); QCOMPARE(px(), b); d.undoStack()->undo(); QCOMPARE(px(), a); }
    void twoStrokesUndoRedoExact() { Document d(makeSprite({8,8})); QRect r(1,1,1,1);
      auto stroke = [&](QRgb c) { QImage b = d.liveCel(0,0).copy(r); d.liveCel(0,0).setPixel(1,1,c); d.commitCelEdit("P",0,0,r,b); };
      auto px = [&] { return d.sprite().layers[0].cels[0].pixel(1,1); };
      const QRgb a = qRgba(10,200,30,77), b = qRgba(250,5,99,201);
      stroke(a); stroke(b); QCOMPARE(px(), b);
      d.undoStack()->undo(); QCOMPARE(px(), a); d.undoStack()->undo(); QCOMPARE(qAlpha(px()), 0);
      d.undoStack()->redo(); QCOMPARE(px(), a); d.undoStack()->redo(); QCOMPARE(px(), b); }
    void snapshotThenStrokeUndoTwice() { Document d(makeSprite({8,8})); d.liveCel(0,0).setPixel(0,0,qRgba(10,200,30,77));
      const QImage orig = d.sprite().layers[0].cels[0].copy();
      d.apply("Add Layer", addLayer(d.sprite(),1));
      QRect r(0,0,1,1); QImage b = d.liveCel(0,0).copy(r); d.liveCel(0,0).setPixel(0,0,qRgba(1,2,3,200));
      d.commitCelEdit("P",0,0,r,b);
      d.undoStack()->undo(); d.undoStack()->undo();
      QCOMPARE(d.sprite().layers.size(), 1); QCOMPARE(d.sprite().layers[0].cels[0], orig); }
};
QTEST_MAIN(TestDocument)
#include "test_document.moc"
