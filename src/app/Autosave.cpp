#include "Autosave.h"
#include "MainWindow.h"
#include "io/SprxIO.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

static QString recoveryDir() { return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1String("/recovery"); }

// shortcut: a second running instance sees the first one's live files as pending recoveries; a QLockFile per session is the upgrade.
Autosave::Autosave(MainWindow *window) : QObject(window), m_window(window)
{
    m_base = recoveryDir() + QLatin1Char('/') + QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_timer.setInterval(3 * 60 * 1000);
    connect(&m_timer, &QTimer::timeout, this, &Autosave::write);
}

void Autosave::write()
{
    const spx::Document *doc = m_window->document();
    if (!doc->isModified()) return;
    // shortcut: floating selection pixels are not dropped first (that would alter live editing state), so they are missing from the recovery file; fine until recovery must be exact.
    QDir().mkpath(recoveryDir());
    QString error;
    if (!spx::saveSprx(doc->sprite(), m_base + QLatin1String(".sprx"), &error)) return; // silent: the next tick retries
    QSaveFile f(m_base + QLatin1String(".json"));
    if (!f.open(QIODevice::WriteOnly)) return;
    const QJsonObject meta{{QStringLiteral("originalPath"), doc->filePath}, {QStringLiteral("savedAt"), QDateTime::currentDateTime().toString(Qt::ISODate)}};
    f.write(QJsonDocument(meta).toJson());
    f.commit();
}

void Autosave::clear() { discard(m_base + QLatin1String(".sprx")); }

QStringList Autosave::pendingRecoveries()
{
    QStringList out;
    for (const QFileInfo &fi : QDir(recoveryDir()).entryInfoList({QStringLiteral("*.sprx")}, QDir::Files, QDir::Time))
        out << fi.absoluteFilePath();
    return out;
}

QJsonObject Autosave::info(const QString &recoveryPath)
{
    QFile f(QString(recoveryPath).replace(QLatin1String(".sprx"), QLatin1String(".json")));
    return f.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(f.readAll()).object() : QJsonObject();
}

void Autosave::discard(const QString &recoveryPath)
{
    QFile::remove(recoveryPath);
    QFile::remove(QString(recoveryPath).replace(QLatin1String(".sprx"), QLatin1String(".json")));
}
