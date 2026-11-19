#include "Compositor.h"
#include "tools/EyedropperTool.h"
#include "tools/FillTool.h"
#include "tools/PencilTool.h"
#include "tools/ShapeTool.h"
#include <QtTest>

using namespace spx;

class TestTools : public QObject
{
    Q_OBJECT
    static QImage& cel(Document& d) { return d.liveCel(0, 0); }
    static bool set(Document& d, int x, int y) { return qAlpha(cel(d).pixel(x, y)) == 255; }
private slots:
    void pencilStrokeIsOneUndo() {
        Document d(makeSprite({8, 8})); ToolContext c{&d}; c.primary = Qt::red; PencilTool p(false);
        p.press(c, {0, 0}, Qt::LeftButton); p.move(c, {3, 0}); p.release(c, {3, 0});
        QCOMPARE(d.sprite().layers[0].cels[0].pixel(3, 0), QColor(Qt::red).rgba()); QCOMPARE(d.undoStack()->count(), 1);
        d.undoStack()->undo(); QCOMPARE(qAlpha(d.sprite().layers[0].cels[0].pixel(3, 0)), 0);
    }
    void rightButtonUsesSecondary() {
        Document d(makeSprite({4, 4})); ToolContext c{&d}; c.secondary = Qt::blue; PencilTool p(false);
        p.press(c, {1, 1}, Qt::RightButton); p.release(c, {1, 1});
        QCOMPARE(d.sprite().layers[0].cels[0].pixel(1, 1), QColor(Qt::blue).rgba());
    }
    void otherButtonsIgnored() {
        Document d(makeSprite({4, 4})); ToolContext c{&d}; PencilTool p(false);
        p.press(c, {1, 1}, Qt::MiddleButton); p.release(c, {1, 1}); QCOMPARE(d.undoStack()->count(), 0);
    }
    void pencilMirror() {
        Document d(makeSprite({4, 4})); ToolContext c{&d}; c.mirrorH = true; PencilTool p(false);
        p.press(c, {0, 1}, Qt::LeftButton); p.release(c, {0, 1}); QCOMPARE(qAlpha(d.sprite().layers[0].cels[0].pixel(3, 1)), 255);
    }
    void pencilOffCanvasNoCrash() {
        Document d(makeSprite({4, 4})); ToolContext c{&d}; PencilTool p(false);
        p.press(c, {-5, -5}, Qt::LeftButton); p.move(c, {10, 10}); p.release(c, {10, 10});
        QCOMPARE(qAlpha(d.sprite().layers[0].cels[0].pixel(2, 2)), 255);
    }
    void fullyOffCanvasNoUndo() {
        Document d(makeSprite({4, 4})); ToolContext c{&d}; PencilTool p(false);
        p.press(c, {-5, -5}, Qt::LeftButton); p.release(c, {-4, -5}); QCOMPARE(d.undoStack()->count(), 0);
    }
    void eraserClears() {
        Document d(makeSprite({4, 4})); cel(d).fill(Qt::red); ToolContext c{&d}; PencilTool p(true);
        p.press(c, {1, 1}, Qt::LeftButton); p.release(c, {1, 1});
        QCOMPARE(qAlpha(cel(d).pixel(1, 1)), 0); QVERIFY(set(d, 0, 0)); QCOMPARE(d.undoStack()->count(), 1);
    }
    void pressWithoutReleaseIsUndoable() {
        Document d(makeSprite({8, 8})); ToolContext c{&d}; PencilTool p(false);
        p.press(c, {0, 0}, Qt::LeftButton); p.move(c, {3, 0});
        p.press(c, {0, 5}, Qt::LeftButton); p.move(c, {3, 5}); p.release(c, {3, 5});
        d.undoStack()->undo(); d.undoStack()->undo();
        QCOMPARE(d.undoStack()->count(), 2);
        for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x) QCOMPARE(qAlpha(cel(d).pixel(x, y)), 0);
    }
    void eraserMirror() {
        Document d(makeSprite({4, 4})); cel(d).fill(Qt::red); ToolContext c{&d}; c.mirrorH = true; PencilTool p(true);
        p.press(c, {0, 1}, Qt::LeftButton); p.release(c, {0, 1});
        QCOMPARE(qAlpha(cel(d).pixel(0, 1)), 0); QCOMPARE(qAlpha(cel(d).pixel(3, 1)), 0); QVERIFY(set(d, 1, 1));
    }
    void strokeLeavesAndReenters() {
        Document d(makeSprite({8, 8})); ToolContext c{&d}; c.pixelPerfect = false; PencilTool p(false);
        p.press(c, {1, 1}, Qt::LeftButton); p.move(c, {-3, 1}); p.move(c, {2, 2}); p.release(c, {2, 2});
        QVERIFY(set(d, 1, 1)); QVERIFY(set(d, 0, 1)); QVERIFY(set(d, 2, 2)); QCOMPARE(d.undoStack()->count(), 1);
    }
    void pixelPerfectStroke() {
        for (bool pp : {true, false}) {
            Document d(makeSprite({4, 4})); ToolContext c{&d}; c.pixelPerfect = pp; PencilTool p(false);
            p.press(c, {0, 0}, Qt::LeftButton); p.move(c, {1, 0}); p.move(c, {1, 1}); p.release(c, {1, 1});
            QCOMPARE(set(d, 1, 0), !pp); QVERIFY(set(d, 0, 0)); QVERIFY(set(d, 1, 1));
        }
    }
    void rectToolOnlyCommitsOnRelease() {
        Document d(makeSprite({8, 8})); ToolContext c{&d}; ShapeTool t(ShapeTool::Rect);
        t.press(c, {0, 0}, Qt::LeftButton); t.move(c, {3, 3}); QVERIFY(!t.preview().isEmpty()); QCOMPARE(d.undoStack()->count(), 0);
        QVERIFY(!set(d, 0, 0));
        t.release(c, {3, 3}); QCOMPARE(d.undoStack()->count(), 1); QCOMPARE(qAlpha(d.sprite().layers[0].cels[0].pixel(1, 1)), 0);
        QVERIFY(set(d, 0, 0));
    }
    void shapeMirror() {
        Document d(makeSprite({8, 8})); ToolContext c{&d}; c.mirrorH = true; ShapeTool t(ShapeTool::Line);
        t.press(c, {0, 0}, Qt::LeftButton); t.move(c, {2, 0}); QVERIFY(t.preview().contains(QPoint(7, 0)));
        t.release(c, {2, 0}); QVERIFY(set(d, 7, 0)); QVERIFY(set(d, 5, 0)); QCOMPARE(d.undoStack()->count(), 1);
    }
    void constrainSquare() {
        Document d(makeSprite({8, 8})); ToolContext c{&d}; c.constrain = true; ShapeTool t(ShapeTool::Rect);
        t.press(c, {0, 0}, Qt::LeftButton); t.release(c, {4, 2});
        QVERIFY(set(d, 4, 4)); QVERIFY(set(d, 0, 4)); QVERIFY(!set(d, 2, 2)); QVERIFY(!set(d, 5, 4));
    }
    void eyedropperPicksComposite() {
        Document d(makeSprite({4, 4})); cel(d).setPixel(0, 0, QColor(Qt::red).rgba());
        ToolContext c{&d}; EyedropperTool e;
        e.press(c, {0, 0}, Qt::LeftButton); QCOMPARE(c.primary, QColor(Qt::red));
        e.press(c, {0, 0}, Qt::RightButton); QCOMPARE(c.secondary, QColor(Qt::red));
        c.primary = Qt::green; e.press(c, {9, 9}, Qt::LeftButton); QCOMPARE(c.primary, QColor(Qt::green));
        QCOMPARE(d.undoStack()->count(), 0);
    }
    void fillTool() {
        Document d(makeSprite({4, 4})); ToolContext c{&d}; c.primary = Qt::red; FillTool f;
        f.press(c, {0, 0}, Qt::LeftButton);
        for (int y = 0; y < 4; ++y) for (int x = 0; x < 4; ++x) QCOMPARE(cel(d).pixel(x, y), QColor(Qt::red).rgba());
        QCOMPARE(d.undoStack()->count(), 1);
        f.press(c, {0, 0}, Qt::LeftButton); f.press(c, {-1, 0}, Qt::LeftButton); QCOMPARE(d.undoStack()->count(), 1);
    }
};

QTEST_MAIN(TestTools)
#include "test_tools.moc"
