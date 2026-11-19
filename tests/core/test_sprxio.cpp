#include "io/SprxIO.h"
#include "SpriteOps.h"
#include <QBuffer>
#include <QtTest>
#include <miniz.h>
#ifndef Q_OS_WIN
#include <unistd.h>
#endif

static void writeZip(const QString &path, const QMap<QString, QByteArray> &files)
{
    mz_zip_archive z{};
    QVERIFY(mz_zip_writer_init_heap(&z, 0, 0));
    for (auto it = files.begin(); it != files.end(); ++it)
        QVERIFY(mz_zip_writer_add_mem(&z, it.key().toUtf8().constData(), it.value().constData(), it.value().size(), MZ_DEFAULT_COMPRESSION));
    void *buf = nullptr; size_t n = 0;
    QVERIFY(mz_zip_writer_finalize_heap_archive(&z, &buf, &n));
    QFile f(path); QVERIFY(f.open(QIODevice::WriteOnly)); f.write(static_cast<const char *>(buf), qint64(n)); f.close();
    mz_zip_writer_end(&z); mz_free(buf);
}

static QByteArray png(QSize s)
{
    QImage i(s, QImage::Format_ARGB32); i.fill(Qt::red);
    QByteArray b; QBuffer buf(&b); buf.open(QIODevice::WriteOnly); i.save(&buf, "PNG"); return b;
}

// Single quotes stand in for double quotes so JSON needs no escaping.
static QString J(const char *s) { return QString(s).replace('\'', '"'); }

static QByteArray manifest(const QString &tags, const QString &cels, const QString &extra = {}, int version = 1, int duration = 100)
{
    return QString(J("{'formatVersion':%1,'width':4,'height':4,'layers':[{'name':'L','visible':true,'opacity':255}],'frames':[{'duration':%2,'flat':'flat/0.png'}],'tags':%3,'palette':[],'cels':%4%5}"))
        .arg(version).arg(duration).arg(tags, cels, extra).toUtf8();
}

class TestSprxIO : public QObject
{
    Q_OBJECT
    void expectReject(const QMap<QString, QByteArray> &files, const char *needle = nullptr) {
        QTemporaryDir t; QString p = t.filePath("x.sprx"); writeZip(p, files);
        QString e; QVERIFY(!spx::loadSprx(p, &e)); QVERIFY(!e.isEmpty()); if (needle) QVERIFY(e.contains(needle));
        QVERIFY(!spx::loadSprx(p, nullptr));
    }
private slots:
    void roundTrip() { auto s = spx::addFrame(spx::addLayer(spx::makeSprite({16,8}),1),1);
      spx::celForWrite(s,1,1).setPixel(3,4,qRgba(10,20,30,200)); s.layers[1].opacity = 100; s.frames[1].durationMs = 250;
      s = spx::addTag(s,{"walk",0,1}); s.sliceGrid = spx::SliceGrid{8,8,0,0,0,0};
      QTemporaryDir t; QString p = t.filePath("a.sprx"); QString e; QVERIFY(spx::saveSprx(s,p,&e));
      auto l = spx::loadSprx(p,&e); QVERIFY2(l, qPrintable(e));
      QCOMPARE(l->size, QSize(16,8)); QCOMPARE(l->layers[1].cels[1].pixel(3,4), qRgba(10,20,30,200)); QVERIFY(l->layers[0].cels[0].isNull());
      QCOMPARE(l->layers[1].opacity,100); QCOMPARE(l->frames[1].durationMs,250); QCOMPARE(l->tags[0].name,"walk");
      QCOMPARE(l->sliceGrid->cellWidth,8); QCOMPARE(l->palette, s.palette); }
    void noGridRoundTrips() { QTemporaryDir t; QString p = t.filePath("a.sprx"); QString e;
      QVERIFY(spx::saveSprx(spx::makeSprite({4,4}),p,&e)); auto l = spx::loadSprx(p,&e); QVERIFY(l); QVERIFY(!l->sliceGrid); }
    void rejectsGarbage() { QTemporaryDir t; QString p = t.filePath("x.sprx"); QFile f(p); QVERIFY(f.open(QIODevice::WriteOnly)); f.write("not a zip"); f.close();
      QString e; QVERIFY(!spx::loadSprx(p,&e)); QVERIFY(!e.isEmpty()); }
    void rejectsMissingFile() { QString e; QVERIFY(!spx::loadSprx("/nonexistent/a.sprx",&e)); QVERIFY(!e.isEmpty()); }
    void rejectsNewerVersion() { expectReject({{"manifest.json", manifest("[]","[]",{},2)}}, "newer"); }
    void rejectsOutOfRangeCel() { expectReject({{"manifest.json", manifest("[]",J("[{'layer':5,'frame':0,'file':'cels/5_0.png'}]"))}}); }
    void rejectsWrongSizeCel() { expectReject({{"manifest.json", manifest("[]",J("[{'layer':0,'frame':0,'file':'cels/0_0.png'}]"))}, {"cels/0_0.png", png({3,4})}}); }
    void rejectsMissingCelPng() { expectReject({{"manifest.json", manifest("[]",J("[{'layer':0,'frame':0,'file':'cels/0_0.png'}]"))}}); }
    void rejectsBadTag() { expectReject({{"manifest.json", manifest(J("[{'name':'t','from':1,'to':0}]"),"[]")}}); }
    void rejectsTagOutOfRange() { expectReject({{"manifest.json", manifest(J("[{'name':'t','from':0,'to':1}]"),"[]")}}); }
    void rejectsBadDuration() { expectReject({{"manifest.json", manifest("[]","[]",{},1,0)}}); }
    void rejectsOversizedManifest() { expectReject({{"manifest.json", manifest("[]","[]") + QByteArray(17 << 20, ' ')}}); }
    void rejectsDuplicateCel() { const QString c = J("{'layer':0,'frame':0,'file':'cels/0_0.png'}");
      expectReject({{"manifest.json", manifest("[]","[" + c + "," + c + "]")}, {"cels/0_0.png", png({4,4})}}); }
    void rejectsTooManyLayers() { QString ls; for (int i = 0; i < 256; ++i) ls += J("{'name':'L','visible':true,'opacity':255},"); // 257 with the manifest's own
      const QByteArray k = J("'layers':[").toUtf8();
      expectReject({{"manifest.json", manifest("[]","[]").replace(k, k + ls.toUtf8())}}); }
    void rejectsBadOpacity() { expectReject({{"manifest.json", manifest("[]","[]").replace(J("'opacity':255").toUtf8(), J("'opacity':300").toUtf8())}}); }
    void rejectsBadSliceGrid() { expectReject({{"manifest.json", manifest("[]","[]", J(",'sliceGrid':{'cellWidth':0,'cellHeight':8}"))}}); }
    void rejectsCelWithoutFile() { expectReject({{"manifest.json", manifest("[]", J("[{'layer':0,'frame':0}]"))}}); }
    void rejectsBadPalette() { expectReject({{"manifest.json", manifest("[]","[]").replace(J("'palette':[]").toUtf8(), J("'palette':['red']").toUtf8())}}); }
    void rejectsHugeCanvas() { expectReject({{"manifest.json", manifest("[]","[]").replace(J("'width':4").toUtf8(), J("'width':2049").toUtf8())}}); }
    void toleratesUnknownKeysAndUpperPalette() { QTemporaryDir t; QString p = t.filePath("x.sprx");
      writeZip(p, {{"manifest.json", manifest("[]","[]").replace(J("'palette':[]").toUtf8(), J("'palette':['#AABBCCDD'],'future':1").toUtf8())}});
      QString e; auto l = spx::loadSprx(p,&e); QVERIFY2(l, qPrintable(e)); QCOMPARE(l->palette[0], QColor(0xaa,0xbb,0xcc,0xdd)); }
    void failedSaveKeepsOldFile() {
#ifdef Q_OS_WIN
      QSKIP("POSIX permission model");
#else
      if (geteuid() == 0) QSKIP("root ignores permissions");
      QTemporaryDir t; QString p = t.filePath("a.sprx"); QString e;
      QVERIFY(spx::saveSprx(spx::makeSprite({4,4}),p,&e)); QFile::setPermissions(p, QFile::ReadOwner);
      QFile(t.path()).setPermissions(QFile::ReadOwner|QFile::ExeOwner);
      QVERIFY(!spx::saveSprx(spx::makeSprite({8,8}),p,&e)); QVERIFY(!e.isEmpty());
      QFile(t.path()).setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner);
      QCOMPARE(spx::loadSprx(p,&e)->size, QSize(4,4));
#endif
    }
    void saveToMissingFolderFails() { QString e; QVERIFY(!spx::saveSprx(spx::makeSprite({4,4}),"/nonexistent/dir/a.sprx",&e)); QVERIFY(!e.isEmpty());
      QVERIFY(!spx::saveSprx(spx::makeSprite({4,4}),"/nonexistent/dir/a.sprx",nullptr)); }
};

QTEST_MAIN(TestSprxIO)
#include "test_sprxio.moc"
