#include "Commands.h"
#include <QPainter>

namespace spx {

SnapshotCommand::SnapshotCommand(Document *d, const QString &text, Sprite before, Sprite after)
    : QUndoCommand(text), m_doc(d), m_before(std::move(before)), m_after(std::move(after)) {}
void SnapshotCommand::undo() { m_doc->setSprite(m_before); }
void SnapshotCommand::redo() { m_doc->setSprite(m_after); }

CelPixelsCommand::CelPixelsCommand(Document *d, const QString &text, int layer, int frame, QRect rect, QImage before, QImage after)
    : QUndoCommand(text), m_doc(d), m_layer(layer), m_frame(frame), m_rect(rect),
      m_before(std::move(before)), m_after(std::move(after)) {}

void CelPixelsCommand::paint(const QImage &img)
{
    QPainter p(&celForWrite(m_doc->m_sprite, m_layer, m_frame));
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawImage(m_rect.topLeft(), img);
    p.end();
    emit m_doc->changed(m_rect);
}
void CelPixelsCommand::undo() { paint(m_before); }
void CelPixelsCommand::redo()
{
    if (m_first) { m_first = false; return; }
    paint(m_after);
}

} // namespace spx
