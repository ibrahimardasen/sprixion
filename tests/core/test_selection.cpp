#include "Selection.h"
#include "SpriteOps.h"
#include "tools/PencilTool.h"
#include "tools/SelectTool.h"
#include <QtTest>

using namespace spx;

class TestSelection : public QObject
{
    Q_OBJECT
    static bool set(Document& d, int x, int y) { return qAlpha(d.sprite().layers[0].cels[0].pixel(x, y)) == 255; }
private slots:
    void liftMoveDropIsUndoable() {
        Document d(makeSprite({8, 8})); d.liveCel(0, 0).setPixel(1, 1, QColor(Qt::red).rgba());
        Selection s; s.rect = QRect(1, 1, 1, 1);
        liftSelection(d, s, 0, 0); QVERIFY(s.isFloating()); QVERIFY(!set(d, 1, 1));
        s.floatingPos = {3, 3};
        dropSelection(d, s, 0, 0);
        QVERIFY(set(d, 3, 3)); QVERIFY(!set(d, 1, 1)); QVERIFY(!s.isFloating()); QCOMPARE(s.rect, QRect(3, 3, 1, 1));
        d.undoStack()->undo(); d.undoStack()->undo();
        QVERIFY(set(d, 1, 1)); QVERIFY(!set(d, 3, 3));
    }
    void pastePastCanvasEdgeClipped() {
        Document d(makeSprite({4, 4})); Selection s;
        QImage red(4, 4, QImage::Format_ARGB32); red.fill(Qt::red);
        pasteImage(d, s, 0, 0, red, {2, 2});
        QCOMPARE(d.undoStack()->count(), 0); // nothing written until drop
        dropSelection(d, s, 0, 0);
        QVERIFY(set(d, 3, 3)); QVERIFY(set(d, 2, 2)); QVERIFY(!set(d, 1, 1)); QCOMPARE(s.rect, QRect(2, 2, 2, 2));
    }
    void flipFloating() {
        Document d(makeSprite({4, 4})); Selection s;
        QImage i(2, 1, QImage::Format_ARGB32); i.setPixel(0, 0, QColor(Qt::red).rgba()); i.setPixel(1, 0, QColor(Qt::blue).rgba());
        pasteImage(d, s, 0, 0, i, {0, 0});
        transformSelection(s, QTransform().scale(-1, 1));
        QCOMPARE(s.floating.pixel(0, 0), QColor(Qt::blue).rgba()); QCOMPARE(s.floating.pixel(1, 0), QColor(Qt::red).rgba());
        transformSelection(s, QTransform().rotate(90));
        QCOMPARE(s.rect.size(), QSize(1, 2));
    }
    void copyReturnsPixels() {
        Document d(makeSprite({4, 4})); d.liveCel(0, 0).setPixel(1, 1, QColor(Qt::red).rgba());
        Selection s; s.rect = QRect(1, 1, 2, 1);
        const QImage c = copySelection(d.sprite(), s, 0, 0);
        QCOMPARE(c.size(), QSize(2, 1)); QCOMPARE(c.pixel(0, 0), QColor(Qt::red).rgba());
    }
    void selectToolDragMoves() {
        Document d(makeSprite({8, 8})); ToolContext c{&d}; c.primary = Qt::red; Selection s;
        PencilTool p(false); p.press(c, {1, 1}, Qt::LeftButton); p.release(c, {1, 1});
        SelectTool t(&s);
        t.press(c, {1, 1}, Qt::LeftButton); t.release(c, {1, 1}); // click selects nothing-drag => clears
        QVERIFY(!s.isActive());
        t.press(c, {0, 0}, Qt::LeftButton); t.move(c, {1, 1}); t.release(c, {1, 1});
        QCOMPARE(s.rect, QRect(0, 0, 2, 2));
        t.press(c, {1, 1}, Qt::LeftButton); t.move(c, {2, 1}); t.move(c, {3, 2}); t.release(c, {3, 2});
        QCOMPARE(s.floatingPos, QPoint(2, 1));
        t.press(c, {7, 7}, Qt::LeftButton); t.release(c, {7, 7}); // click outside drops
        QVERIFY(!s.isFloating()); QVERIFY(!s.isActive());
        QVERIFY(set(d, 3, 2)); QVERIFY(!set(d, 1, 1));
        d.undoStack()->undo(); d.undoStack()->undo();
        QVERIFY(set(d, 1, 1)); QVERIFY(!set(d, 3, 2));
    }
};

QTEST_MAIN(TestSelection)
#include "test_selection.moc"
