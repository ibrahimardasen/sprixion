#include <QApplication>
#include <QLocale>
#include <QMainWindow>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Sprixion"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));

    QTranslator translator;
    if (translator.load(QStringLiteral(":/i18n/sprixion_") + QLocale::system().name()))
        app.installTranslator(&translator);

    QMainWindow window;
    window.setWindowTitle(QObject::tr("Sprixion"));
    window.resize(1024, 700);
    window.show();
    return app.exec();
}
