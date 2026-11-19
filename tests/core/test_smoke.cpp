#include <QtTest>

class TestSmoke : public QObject
{
    Q_OBJECT
private slots:
    void works() { QVERIFY(true); }
};

QTEST_APPLESS_MAIN(TestSmoke)
#include "test_smoke.moc"
