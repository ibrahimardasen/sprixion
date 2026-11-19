#include "SpriteOps.h"
#include "io/SheetExport.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QtTest>

using namespace spx;

class TestSheetExport : public QObject
{
    Q_OBJECT
private slots:
    void gridLayout() {
        auto s = makeSprite({8, 4}); for (int i = 0; i < 4; ++i) s = addFrame(s, 1); // 5 frames
        auto r = exportSheet(s, SheetLayout::Grid, 2, "a.png");
        QCOMPARE(r.image.size(), QSize(16, 12));
        auto f = r.json["frames"].toArray(); QCOMPARE(f.size(), 5); const QJsonObject f3 = f[3].toObject();
        QCOMPARE(f3["frame"]["x"].toInt(), 8); QCOMPARE(f3["frame"]["y"].toInt(), 4);
        QCOMPARE(f3["filename"].toString(), QString("a 3.png"));
        QCOMPARE(r.json["meta"]["size"]["w"].toInt(), 16);
    }
    void jsonMetaAndTags() {
        auto s = makeSprite({8, 4}); s = addFrame(s, 1); s = addTag(s, {"walk", 0, 1});
        auto r = exportSheet(s, SheetLayout::Horizontal, 1, "a.png");
        auto m = r.json["meta"];
        QCOMPARE(m["frameTags"][0]["name"].toString(), QString("walk"));
        QCOMPARE(m["frameTags"][0]["direction"].toString(), QString("forward"));
        QCOMPARE(m["app"].toString(), QString("Sprixion")); QCOMPARE(m["image"].toString(), QString("a.png"));
        QCOMPARE(m["version"].toString(), QString("0.1.0")); QCOMPARE(m["scale"].toString(), QString("1"));
        QCOMPARE(r.json["frames"][0]["duration"].toInt(), 100);
    }
    void horizontalLayout() {
        auto s = makeSprite({8, 4}); s = addFrame(s, 1); s = addFrame(s, 1);
        QCOMPARE(exportSheet(s, SheetLayout::Horizontal, 1, "a.png").image.size(), QSize(24, 4));
    }
    void noTagsIsEmptyArray() {
        auto r = exportSheet(makeSprite({2, 2}), SheetLayout::Grid, 99, "a.png");
        QVERIFY(r.json["meta"]["frameTags"].isArray()); QCOMPARE(r.json["meta"]["frameTags"].toArray().size(), 0);
        QCOMPARE(r.image.size(), QSize(2, 2));
    }
};

QTEST_MAIN(TestSheetExport)
#include "test_sheetexport.moc"
