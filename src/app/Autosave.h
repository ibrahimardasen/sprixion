#pragma once
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTimer>

class MainWindow;

// Periodically writes the modified document to <AppData>/recovery/<session>.sprx (+ .json metadata)
// so a crash loses at most one interval of work.
class Autosave : public QObject
{
public:
    explicit Autosave(MainWindow *window);
    void start() { m_timer.start(); }
    void clear(); // delete this session's recovery files
    static QStringList pendingRecoveries(); // recovery .sprx paths, newest first
    static QJsonObject info(const QString &recoveryPath); // {"originalPath", "savedAt"}; empty if unreadable
    static void discard(const QString &recoveryPath); // delete the .sprx and its .json

private:
    void write();
    MainWindow *m_window;
    QTimer m_timer;
    QString m_base; // <dir>/<session uuid>, no suffix
};
