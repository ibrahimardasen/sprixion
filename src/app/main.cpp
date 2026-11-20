#include "MainWindow.h"
#include <QApplication>
#include <QFile>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Sprixion"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setStyle(QStringLiteral("Fusion"));
    QFile qss(QStringLiteral(":/theme.qss"));
    if (qss.open(QIODevice::ReadOnly)) app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    QTranslator translator;
    if (translator.load(QStringLiteral(":/i18n/sprixion_") + QLocale::system().name()))
        app.installTranslator(&translator);

    MainWindow window;
    window.show();
    window.offerRecovery();
    return app.exec();
}
