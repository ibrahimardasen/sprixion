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
    void touchAndCelUndoEmitDirtyRect() { Document d(makeSprite({8,8})); QSignalSpy spy(&d, &Document::changed);
      d.touch(QRect(1,1,2,2)); QCOMPARE(spy.takeFirst().at(0).toRect(), QRect(1,1,2,2));
      QRect r(2,2,1,1); QImage b = d.liveCel(0,0).copy(r); d.liveCel(0,0).setPixel(2,2,qRgba(1,2,3,255));
      d.commitCelEdit("P",0,0,r,b); QCOMPARE(spy.count(),0);
      d.undoStack()->undo(); QCOMPARE(spy.takeFirst().at(0).toRect(), r); }
};
QTEST_MAIN(TestDocument)
#include "test_document.moc"
