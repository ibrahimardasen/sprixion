#include "io/PaletteIO.h"
#include <QTemporaryDir>
#include <QtTest>

using namespace spx;

class TestPaletteIO : public QObject
{
    Q_OBJECT
    QTemporaryDir dir;
    QString write(const QString& name, const QByteArray& data) {
        const QString p = dir.filePath(name); QFile f(p); if (!f.open(QIODevice::WriteOnly)) qFatal("open"); f.write(data); return p;
    }
private slots:
    void loadsGpl() {
        auto r = loadPalette(write("a.GPL", "GIMP Palette\nName: x\n#c\n255 0 0 Red\n0 0 255\n"), nullptr);
        QVERIFY(r); QCOMPARE(r->size(), 2); QCOMPARE((*r)[0], QColor(Qt::red));
    }
    void loadsHex() {
        auto r = loadPalette(write("a.hex", "ff0000\n#00ff00\n\n"), nullptr);
        QVERIFY(r); QCOMPARE(r->size(), 2); QCOMPARE((*r)[1], QColor(Qt::green));
    }
    void rejectsBadLine() {
        QString e;
        QVERIFY(!loadPalette(write("b.hex", "zz0000\n"), &e)); QVERIFY(e.contains("1"));
        QVERIFY(!loadPalette(write("c.hex", "ff0000\nfff\n"), &e)); QVERIFY(e.contains("2"));
        QVERIFY(!loadPalette(write("d.gpl", "GIMP Palette\n256 0 0\n"), &e)); QVERIFY(e.contains("2"));
        QVERIFY(!loadPalette(write("e.gpl", "GIMP Palette\n1 2\n"), &e)); QVERIFY(e.contains("2"));
        QVERIFY(!loadPalette(write("f.gpl", "Nope\n1 2 3\n"), &e));
    }
    void rejectsUnknownAndHuge() {
        QString e;
        QVERIFY(!loadPalette(write("a.txt", "ff0000\n"), &e)); QVERIFY(!e.isEmpty());
        QVERIFY(!loadPalette(write("big.hex", QByteArray(2'000'000, '\n')), &e));
        QVERIFY(!loadPalette(dir.filePath("missing.hex"), nullptr));
    }
};

QTEST_MAIN(TestPaletteIO)
#include "test_paletteio.moc"
