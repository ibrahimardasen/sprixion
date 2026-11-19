#include "Document.h"
#include "Commands.h"

namespace spx {

Document::Document(Sprite s, QObject *parent)
    : QObject(parent), m_sprite(std::move(s)), m_stack(new QUndoStack(this)) {}

void Document::setSprite(Sprite s)
{
    m_sprite = std::move(s);
    emit changed(QRect());
}

void Document::apply(const QString &text, Sprite after)
{
    m_stack->push(new SnapshotCommand(this, text, m_sprite, std::move(after)));
}

QImage &Document::liveCel(int layer, int frame)
{
    Q_ASSERT(layer >= 0 && layer < m_sprite.layers.size() && frame >= 0 && frame < m_sprite.frames.size());
    return celForWrite(m_sprite, layer, frame);
}

void Document::commitCelEdit(const QString &text, int layer, int frame, const QRect &rect, const QImage &before)
{
    QImage after = liveCel(layer, frame).copy(rect);
    m_stack->push(new CelPixelsCommand(this, text, layer, frame, rect, before, after));
}

} // namespace spx
