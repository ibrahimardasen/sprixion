#pragma once
#include "Sprite.h"
#include <QImage>
#include <QObject>
#include <QRect>
#include <QString>
#include <QUndoStack>

namespace spx {

class Document : public QObject
{
    Q_OBJECT
public:
    explicit Document(Sprite s, QObject *parent = nullptr);
    const Sprite &sprite() const { return m_sprite; }
    QUndoStack *undoStack() { return m_stack; }
    // Pushes a whole-sprite snapshot command (QImage sharing keeps it cheap).
    void apply(const QString &text, Sprite after);
    // Mutable cel access during a stroke (creates the cel); caller then calls touch().
    QImage &liveCel(int layer, int frame);
    void touch(const QRect &dirty) { emit changed(dirty); }
    // `before` = rect-sized pixels before the stroke; the live cel already holds the result.
    void commitCelEdit(const QString &text, int layer, int frame, const QRect &rect, const QImage &before);
    bool isModified() const { return !m_stack->isClean(); }

    QString filePath;

signals:
    void changed(QRect dirty); // null rect = everything / structure changed

private:
    friend class SnapshotCommand;
    friend class CelPixelsCommand;
    void setSprite(Sprite s);
    Sprite m_sprite;
    QUndoStack *m_stack;
};

} // namespace spx
